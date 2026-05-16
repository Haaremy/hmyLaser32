#include "EspNow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>
#include "Bridge.h"
#include "Config.h"
#include "Globals.h"

static bool espNowReady = false;
static const uint8_t kBroadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static int findSnapshotById(uint32_t playerId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] != '\0' && g_snapshots[i].playerId == playerId) return i;
  }
  return -1;
}

static int findFreeSnapshot() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] == '\0') return i;
  }
  return -1;
}

static void diffAndForward(const Message &m) {
  for (int i = 0; i < m.playerCount && i < MAX_PLAYERS; i++) {
    const RankingEntry &e = m.entries[i];
    if (e.player[0] == '\0') continue;

    int idx = findSnapshotById(e.playerId);
    if (idx < 0) {
      idx = findFreeSnapshot();
      if (idx < 0) continue;
      g_snapshots[idx].playerId = e.playerId;
      strlcpy(g_snapshots[idx].player, e.player, sizeof(g_snapshots[idx].player));
      g_snapshots[idx].lastPoints = e.points;
      g_snapshots[idx].lastUpdate = e.lastUpdate;
      continue;
    }

    if (e.lastUpdate <= g_snapshots[idx].lastUpdate) continue;

    int delta = e.points - g_snapshots[idx].lastPoints;
    g_snapshots[idx].lastPoints = e.points;
    g_snapshots[idx].lastUpdate = e.lastUpdate;
    strlcpy(g_snapshots[idx].player, e.player, sizeof(g_snapshots[idx].player));

    if (delta > 0 && g_matchPhase == MATCH_ACTIVE) {
      char shooter[20];
      snprintf(shooter, sizeof(shooter), "nec:%08lx", (unsigned long)e.playerId);
      bridgeForwardHit(shooter, "", delta);
      LT_LOG("Hit forwarded: %s +%d", e.player, delta);
    }
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
#else
static void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
#endif
  if (len != sizeof(Message)) return;
  Message m;
  memcpy(&m, data, sizeof(m));
  if (m.msgType == MSG_TABLE) {
    diffAndForward(m);
  }
  // MSG_DISCOVERY/MSG_PHASE ignorieren (Server hört eigene Echos nicht weiter aus)
}

void espNowBegin() {
  if (espNowReady) return;
  if (esp_now_init() != ESP_OK) {
    LT_LOG("esp_now_init failed");
    return;
  }
  esp_now_register_recv_cb(onRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcast, 6);
  peer.channel = 0;          // 0 = aktueller Channel
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    LT_LOG("broadcast peer add failed (ok if already present)");
  }
  espNowReady = true;
  LT_LOG("ESP-NOW ready (channel %d)", WiFi.channel());
}

void espNowLoop() { /* Callback-driven */ }

void espNowBroadcastPhase() {
  if (!espNowReady) return;
  Message m = {};
  strlcpy(m.senderName, g_serverName, sizeof(m.senderName));
  m.msgType = MSG_PHASE;
  m.playerCount = 1;

  m.entries[0].playerId = (uint32_t)g_matchPhase;
  // Restzeit in Sekunden je nach Phase
  uint32_t secsLeft = 0;
  if (g_matchPhase == MATCH_LOBBY) {
    long left = (long)(g_lobbyEndsAtMs - millis()) / 1000L;
    secsLeft = left > 0 ? (uint32_t)left : 0;
  } else if (g_matchPhase == MATCH_ACTIVE) {
    long left = (long)(g_matchEndsAtMs - millis()) / 1000L;
    secsLeft = left > 0 ? (uint32_t)left : 0;
  }
  m.entries[0].points = (int16_t)(secsLeft > 32767 ? 32767 : secsLeft);
  strlcpy(m.entries[0].player, g_settings.mode, sizeof(m.entries[0].player));
  m.entries[0].lastUpdate = millis();

  esp_now_send(kBroadcast, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
}

int espNowGetRanking(PlayerSnapshot *out) {
  int count = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] != '\0') {
      out[count++] = g_snapshots[i];
    }
  }
  return count;
}
