#include <WiFi.h>
#include <cstring>
#include "Config.h"
#include "Display.h"
#include "Globals.h"
#include "Identity.h"
#include "LasertagNetwork.h"
#include "Led.h"
#include "Ranking.h"

bool peerExists(const uint8_t *mac) {
  for (int i = 0; i < peerCount; i++) {
    if (memcmp(knownPeers[i], mac, 6) == 0) return true;
  }
  return false;
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
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info; lastSendStatus = status; sendStatusPending = true;
}
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr; lastSendStatus = status; sendStatusPending = true;
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
    if (newPhase == PHASE_LOBBY) {
      shotsFired = 0; hitCount = 0; myPoints = 0;
      memset(ranking, 0, sizeof(ranking));
    }
    applyTeamColor();
    updateDisplay();
  }
}

static void handleTeams(const Message &m) {
  g_teamCount = m.playerCount > MAX_TEAMS ? MAX_TEAMS : m.playerCount;
  g_myTeamIndex = -1;
  uint8_t myCmd = identityMyCommand();
  uint32_t myBit = (myCmd >= 1 && myCmd <= 32) ? (1u << (myCmd - 1)) : 0u;
  for (uint8_t i = 0; i < g_teamCount; i++) {
    strlcpy(g_teams[i].name, m.entries[i].player, sizeof(g_teams[i].name));
    g_teams[i].color      = m.entries[i].playerId & 0x00FFFFFFu;
    g_teams[i].memberBits = m.entries[i].lastUpdate;
    if (myBit && (g_teams[i].memberBits & myBit)) g_myTeamIndex = (int8_t)i;
  }
  g_teamsLastUpdate = millis();
  applyTeamColor();
  updateDisplay();
}

// v4.1: MSG_STANDALONE vom Lobby-Master.
// entries[0].playerId   = phase
// entries[0].points     = secondsLeftInPhase
// entries[0].player     = "standalone"
// entries[0].lastUpdate = millis() (Sender-Clock)
static void handleStandalone(const Message &m, const uint8_t *srcMac) {
  if (m.playerCount < 1) return;
  uint8_t newPhase = (uint8_t)m.entries[0].playerId;
  if (newPhase > PHASE_DONE) return;
  unsigned long secsLeft = (uint32_t)(m.entries[0].points < 0 ? 0 : m.entries[0].points);
  unsigned long now = millis();

  uint8_t prevPhase = g_standalonePhase;
  g_standalonePhase = newPhase;
  g_standaloneLastUpdate = now;

  if (newPhase == PHASE_LOBBY) {
    g_standaloneLobbyEnds = now + secsLeft * 1000UL;
  } else if (newPhase == PHASE_ACTIVE) {
    g_standaloneMatchEnds = now + secsLeft * 1000UL;
  }

  if (prevPhase != newPhase) {
    Serial.printf("[STANDALONE] Synced from master: phase %u -> %u (%lu s left)\n",
                  prevPhase, newPhase, secsLeft);
    if (newPhase == PHASE_LOBBY) {
      shotsFired = 0; hitCount = 0; myPoints = 0;
      memset(ranking, 0, sizeof(ranking));
    }
    applyTeamColor();
    updateDisplay();
  }
}

void processIncomingMessage(const uint8_t *srcAddr, const uint8_t *data, int len) {
  if (len != sizeof(Message)) return;
  Message incoming;
  memcpy(&incoming, data, sizeof(incoming));
  addPeer(srcAddr);

  bool isServer = (incoming.msgType == MSG_PHASE || incoming.msgType == MSG_TEAMS);
  identityPeerSeen(srcAddr, isServer);

  switch (incoming.msgType) {
    case MSG_DISCOVERY:
      queueTableBroadcast();
      return;
    case MSG_PHASE:
      handlePhase(incoming);
      return;
    case MSG_TEAMS:
      handleTeams(incoming);
      return;
    case MSG_STANDALONE:
      handleStandalone(incoming, srcAddr);
      return;
    case MSG_NFC:
      return;
    case MSG_TABLE: {
      bool changed = false;
      for (int i = 0; i < incoming.playerCount && i < MAX_PLAYERS; i++) {
        if (incoming.entries[i].player[0] == '\0') continue;
        if (upsertRankingEntry(incoming.entries[i].playerId,
                               incoming.entries[i].player,
                               incoming.entries[i].points,
                               incoming.entries[i].lastUpdate)) changed = true;
      }
      if (changed) {
        syncMyPointsFromRanking();
        updateDisplay();
        queueTableBroadcast();
      }
      return;
    }
    default:
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
  strncpy(outgoing.senderName, identityMyName(), sizeof(outgoing.senderName) - 1);
  outgoing.msgType = MSG_DISCOVERY;
  outgoing.playerCount = 0;
  esp_now_send(broadcast, reinterpret_cast<const uint8_t *>(&outgoing), sizeof(outgoing));
}

void broadcastRankingTable() {
  const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memset(&outgoing, 0, sizeof(outgoing));
  strncpy(outgoing.senderName, identityMyName(), sizeof(outgoing.senderName) - 1);
  outgoing.msgType = MSG_TABLE;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    outgoing.entries[i] = ranking[i];
    if (ranking[i].player[0] != '\0') outgoing.playerCount++;
  }
  // Broadcast statt unicast-an-jeden — spart Bandbreite
  esp_now_send(broadcast, reinterpret_cast<const uint8_t *>(&outgoing), sizeof(outgoing));
  lastTableBroadcast = millis();
  pendingTableBroadcast = false;
}

// v4.1 — Master broadcastet aktuelle Standalone-Phase + Restzeit
void broadcastStandaloneState() {
  if (!identityIsLobbyMaster()) return;
  if (g_standalonePhase != PHASE_LOBBY && g_standalonePhase != PHASE_ACTIVE) return;

  const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  Message m = {};
  strlcpy(m.senderName, identityMyName(), sizeof(m.senderName));
  m.msgType = MSG_STANDALONE;
  m.playerCount = 1;
  m.entries[0].playerId = (uint32_t)g_standalonePhase;

  unsigned long now = millis();
  long secsLeft = 0;
  if (g_standalonePhase == PHASE_LOBBY) {
    secsLeft = (long)(g_standaloneLobbyEnds - now) / 1000L;
  } else if (g_standalonePhase == PHASE_ACTIVE) {
    secsLeft = (long)(g_standaloneMatchEnds - now) / 1000L;
  }
  if (secsLeft < 0) secsLeft = 0;
  m.entries[0].points = (int16_t)(secsLeft > 32767 ? 32767 : secsLeft);
  strlcpy(m.entries[0].player, "standalone", sizeof(m.entries[0].player));
  m.entries[0].lastUpdate = now;

  esp_now_send(broadcast, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
}

void handleSendStatusLog() {
  if (sendStatusPending) sendStatusPending = false;
}
