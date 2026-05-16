#include "EspNow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>
#include "Bridge.h"
#include "Config.h"
#include "Globals.h"

static bool espNowReady = false;

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

// Score-Diff-Logik: wenn ein bekannter Spieler MEHR Punkte hat als zuletzt
// gespeichert, war das letzte Ereignis ein Treffer durch ihn. Wir forwarden
// das als HitEvent — aber nur, wenn ein Match aktiv ist.
static void diffAndForward(const Message &m) {
  if (g_matchPhase != MATCH_ACTIVE || strlen(g_currentMatchId) == 0) {
    // trotzdem Snapshots updaten, damit pre-match-State frisch ist
  }

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

    if (e.lastUpdate <= g_snapshots[idx].lastUpdate) continue; // älterer Snapshot

    int delta = e.points - g_snapshots[idx].lastPoints;
    g_snapshots[idx].lastPoints = e.points;
    g_snapshots[idx].lastUpdate = e.lastUpdate;
    strlcpy(g_snapshots[idx].player, e.player, sizeof(g_snapshots[idx].player));

    if (delta > 0 && g_matchPhase == MATCH_ACTIVE) {
      // Score-Diff: dieser Spieler hat einen Treffer gelandet.
      // Wir kennen das Target nicht (Score wird vom Empfänger geschrieben);
      // Forward mit shooter=playerName, target unbekannt.
      char shooter[16];
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
  diffAndForward(m);
}

void espNowBegin() {
  if (espNowReady) return;
  // ESP-NOW braucht WiFi-init; im STA-Mode bereits aktiv. Im AP-Mode auch OK.
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    LT_LOG("esp_now_init failed");
    return;
  }
  esp_now_register_recv_cb(onRecv);
  espNowReady = true;
  LT_LOG("ESP-NOW listening on channel %d", WIFI_CHANNEL);
}

void espNowLoop() {
  // Im aktuellen Setup arbeitet ESP-NOW komplett im Callback. Hier wäre Platz
  // für periodisches Pruning oder Statistik.
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
