#include <WiFi.h>
#include <cstring>
#include "Config.h"
#include "Display.h"
#include "Globals.h"
#include "LasertagNetwork.h"
#include "Led.h"
#include "Ranking.h"

bool peerExists(const uint8_t *mac) {
  for (int i = 0; i < peerCount; i++) {
    if (memcmp(knownPeers[i], mac, 6) == 0) return true;
  }
  return false;
}

void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
}

void addPeer(const uint8_t *mac) {
  if (peerExists(mac) || peerCount >= MAX_PEERS) return;
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    memcpy(knownPeers[peerCount], mac, 6);
    peerCount++;
    Serial.print("[ESP-NOW] Peer added: ");
    printMac(mac);
    Serial.println();
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info;
  lastSendStatus = status;
  sendStatusPending = true;
}
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
  lastSendStatus = status;
  sendStatusPending = true;
}
#endif

static void handlePhase(const Message &m) {
  if (m.playerCount < 1) return;
  uint8_t newPhase = (uint8_t)m.entries[0].playerId;
  if (newPhase > PHASE_DONE) return;

  uint8_t prevPhase = g_phase;
  g_phase = newPhase;
  g_phaseSecondsLeft = (uint32_t)(m.entries[0].points < 0 ? 0 : m.entries[0].points);
  g_phaseLastUpdate = millis();
  strlcpy(g_phaseMode, m.entries[0].player, sizeof(g_phaseMode));

  if (prevPhase != newPhase) {
    Serial.printf("[PHASE] %u -> %u (%s, %lu s)\n",
                  prevPhase, newPhase, g_phaseMode, (unsigned long)g_phaseSecondsLeft);
    if (newPhase == PHASE_LOBBY) {
      shotsFired = 0;
      hitCount = 0;
      myPoints = 0;
      memset(ranking, 0, sizeof(ranking));
    }
    // Wenn das neue Phase nicht Team-Mode-relevant ist und keine Teams
    // vom Server kommen, LED auf weiß zurücksetzen (siehe Led.cpp::applyTeamColor)
    applyTeamColor();
    updateDisplay();
  }
}

static void handleTeams(const Message &m) {
  g_teamCount = m.playerCount > MAX_TEAMS ? MAX_TEAMS : m.playerCount;
  g_myTeamIndex = -1;
  uint32_t myBit = (MY_IR_COMMAND >= 1 && MY_IR_COMMAND <= 32) ? (1u << (MY_IR_COMMAND - 1)) : 0u;
  for (uint8_t i = 0; i < g_teamCount; i++) {
    strlcpy(g_teams[i].name, m.entries[i].player, sizeof(g_teams[i].name));
    g_teams[i].color      = m.entries[i].playerId & 0x00FFFFFFu;
    g_teams[i].memberBits = m.entries[i].lastUpdate;
    if (myBit && (g_teams[i].memberBits & myBit)) g_myTeamIndex = (int8_t)i;
  }
  g_teamsLastUpdate = millis();
  Serial.printf("[TEAMS] %u teams; myIndex=%d\n", g_teamCount, g_myTeamIndex);
  applyTeamColor();
  updateDisplay();
}

void processIncomingMessage(const uint8_t *srcAddr, const uint8_t *data, int len) {
  if (len != sizeof(Message)) {
    Serial.println("[ESP-NOW] Ignored packet with unexpected size");
    return;
  }
  Message incoming;
  memcpy(&incoming, data, sizeof(incoming));
  addPeer(srcAddr);

  switch (incoming.msgType) {
    case MSG_DISCOVERY:
      Serial.print("[ESP-NOW] Discovery from ");
      Serial.println(incoming.senderName);
      queueTableBroadcast();
      return;

    case MSG_PHASE:
      handlePhase(incoming);
      return;

    case MSG_TEAMS:
      handleTeams(incoming);
      return;

    case MSG_TABLE: {
      bool changed = false;
      for (int i = 0; i < incoming.playerCount && i < MAX_PLAYERS; i++) {
        if (incoming.entries[i].player[0] == '\0') continue;
        if (upsertRankingEntry(incoming.entries[i].playerId,
                               incoming.entries[i].player,
                               incoming.entries[i].points,
                               incoming.entries[i].lastUpdate)) {
          changed = true;
        }
      }
      if (changed) {
        syncMyPointsFromRanking();
        printRanking();
        updateDisplay();
        queueTableBroadcast();
      }
      return;
    }

    default:
      Serial.printf("[ESP-NOW] Unknown msgType=%u ignored\n", incoming.msgType);
      return;
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  processIncomingMessage(info->src_addr, data, len);
}
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  processIncomingMessage(mac, data, len);
}
#endif

void sendDiscovery() {
  const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memset(&outgoing, 0, sizeof(outgoing));
  strncpy(outgoing.senderName, MY_NAME, sizeof(outgoing.senderName) - 1);
  outgoing.msgType = MSG_DISCOVERY;
  outgoing.playerCount = 0;
  esp_now_send(broadcast, reinterpret_cast<const uint8_t *>(&outgoing), sizeof(outgoing));
}

void broadcastRankingTable() {
  memset(&outgoing, 0, sizeof(outgoing));
  strncpy(outgoing.senderName, MY_NAME, sizeof(outgoing.senderName) - 1);
  outgoing.msgType = MSG_TABLE;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    outgoing.entries[i] = ranking[i];
    if (ranking[i].player[0] != '\0') outgoing.playerCount++;
  }
  for (int i = 0; i < peerCount; i++) {
    esp_now_send(knownPeers[i], reinterpret_cast<const uint8_t *>(&outgoing), sizeof(outgoing));
  }
  lastTableBroadcast = millis();
  pendingTableBroadcast = false;
}

void handleSendStatusLog() {
  if (sendStatusPending) {
    sendStatusPending = false;
    Serial.print("[ESP-NOW] Send status: ");
    Serial.println(lastSendStatus == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  }
}
