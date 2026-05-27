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
static const uint32_t COLOR_POOL[10] = {
  0xdc2626, 0x2563eb, 0x16a34a, 0xd97706, 0x9333ea,
  0x0891b2, 0xeab308, 0xec4899, 0x64748b, 0xffffff
};

static int findSnapshotById(uint32_t playerId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] != '\0' && g_snapshots[i].playerId == playerId) return i;
  }
  return -1;
}
static int findFreeSnapshot() {
  for (int i = 0; i < MAX_PLAYERS; i++) if (g_snapshots[i].player[0] == '\0') return i;
  return -1;
}

static uint8_t necCommand(uint32_t playerId) {
  return (uint8_t)((playerId >> 8) & 0xFFu);
}

static uint32_t defaultColorForPlayer(uint32_t playerId, int idx) {
  uint8_t cmd = necCommand(playerId);
  if (cmd >= 1) return COLOR_POOL[(cmd - 1) % 10];
  return COLOR_POOL[idx % 10];
}

static int teamIndexForCommand(uint8_t cmd) {
  if (cmd < 1 || cmd > 32) return -1;
  uint32_t bit = 1u << (cmd - 1);
  for (uint8_t i = 0; i < g_settings.teamCount; i++) {
    if (g_settings.teams[i].memberBits & bit) return i;
  }
  return -1;
}

static const char *tokenOrNec(const PlayerSnapshot &s, char *buf, size_t len) {
  if (s.nfcToken[0] != '\0') return s.nfcToken;
  snprintf(buf, len, "nec:%08lx", (unsigned long)s.playerId);
  return buf;
}

static PlayerSnapshot *ensureSnapshot(uint32_t playerId, const char *name) {
  int idx = findSnapshotById(playerId);
  if (idx < 0) {
    idx = findFreeSnapshot();
    if (idx < 0) return nullptr;
    memset(&g_snapshots[idx], 0, sizeof(g_snapshots[idx]));
    g_snapshots[idx].playerId = playerId;
    g_snapshots[idx].color = defaultColorForPlayer(playerId, idx);
    g_snapshots[idx].teamIndex = (uint8_t)idx;
  }
  if (name && name[0] != '\0') strlcpy(g_snapshots[idx].player, name, sizeof(g_snapshots[idx].player));
  if (g_snapshots[idx].player[0] == '\0') snprintf(g_snapshots[idx].player, sizeof(g_snapshots[idx].player), "P%02u", (unsigned)(idx + 1));
  return &g_snapshots[idx];
}

static void diffAndForward(const Message &m) {
  for (int i = 0; i < m.playerCount && i < MAX_PLAYERS; i++) {
    const RankingEntry &e = m.entries[i];
    if (e.player[0] == '\0') continue;
    int idx = findSnapshotById(e.playerId);
    if (idx < 0) {
      PlayerSnapshot *created = ensureSnapshot(e.playerId, e.player);
      if (!created) continue;
      created->lastPoints = e.points;
      created->lastUpdate = e.lastUpdate;
      continue;
    }
    if (e.lastUpdate <= g_snapshots[idx].lastUpdate) continue;
    int delta = e.points - g_snapshots[idx].lastPoints;
    g_snapshots[idx].lastPoints = e.points;
    g_snapshots[idx].lastUpdate = e.lastUpdate;
    strlcpy(g_snapshots[idx].player, e.player, sizeof(g_snapshots[idx].player));
    if (delta > 0 && g_matchPhase == MATCH_ACTIVE) {
      char shooter[20];
      bridgeForwardHit(tokenOrNec(g_snapshots[idx], shooter, sizeof(shooter)), "", delta);
      LT_LOG("Hit forwarded: %s +%d", e.player, delta);
    }
  }
}

static void handlePlayerState(const Message &m) {
  if (m.playerCount < 1) return;
  const RankingEntry &base = m.entries[0];
  PlayerSnapshot *s = ensureSnapshot(base.playerId, base.player);
  if (!s) return;
  s->lastPoints = base.points;
  s->lastUpdate = base.lastUpdate;
  if (m.playerCount > 1) {
    if (m.entries[1].playerId != 0) s->color = m.entries[1].playerId & 0x00FFFFFFu;
    s->shotsFired = (uint16_t)(m.entries[1].points < 0 ? 0 : m.entries[1].points);
    s->rxHits = (uint16_t)m.entries[1].lastUpdate;
  }
  if (m.playerCount > 2) {
    s->teamIndex = (uint8_t)(m.entries[2].points < 0 ? 0 : m.entries[2].points);
    char token[40] = {};
    size_t pos = 0;
    for (int i = 2; i < m.playerCount && i < 6 && pos < sizeof(token) - 1; i++) {
      size_t n = strnlen(m.entries[i].player, sizeof(m.entries[i].player));
      if (n > sizeof(token) - 1 - pos) n = sizeof(token) - 1 - pos;
      memcpy(token + pos, m.entries[i].player, n);
      pos += n;
    }
    token[pos] = '\0';
    if (token[0] != '\0') strlcpy(s->nfcToken, token, sizeof(s->nfcToken));
  }
}

static void handleNfc(const Message &m) {
  if (m.playerCount < 1) return;
  PlayerSnapshot *s = ensureSnapshot(m.entries[0].playerId, m.entries[0].player);
  if (!s) return;
  char token[40] = {};
  size_t pos = 0;
  for (int i = 1; i < m.playerCount && i < MAX_PLAYERS && pos < sizeof(token) - 1; i++) {
    size_t n = strnlen(m.entries[i].player, sizeof(m.entries[i].player));
    if (n > sizeof(token) - 1 - pos) n = sizeof(token) - 1 - pos;
    memcpy(token + pos, m.entries[i].player, n);
    pos += n;
  }
  token[pos] = '\0';
  if (token[0] != '\0') strlcpy(s->nfcToken, token, sizeof(s->nfcToken));
  LT_LOG("NFC bound: %s -> %s", s->player, s->nfcToken);
}

static void handleHitEvent(const Message &m) {
  if (m.playerCount < 2 || g_matchPhase != MATCH_ACTIVE) return;
  PlayerSnapshot *shooter = ensureSnapshot(m.entries[0].playerId, m.entries[0].player);
  PlayerSnapshot *target = ensureSnapshot(m.entries[1].playerId, m.entries[1].player);
  if (!shooter || !target) return;
  if (m.entries[0].lastUpdate <= shooter->lastUpdate) return;
  shooter->lastPoints += m.entries[0].points;
  shooter->lastUpdate = m.entries[0].lastUpdate;
  target->rxHits = (uint16_t)(m.entries[1].points < 0 ? 0 : m.entries[1].points);
  target->shotsFired = (uint16_t)m.entries[1].lastUpdate;
  target->lastUpdate = millis();
  char shooterBuf[20];
  char targetBuf[20];
  bridgeForwardHit(tokenOrNec(*shooter, shooterBuf, sizeof(shooterBuf)),
                   tokenOrNec(*target, targetBuf, sizeof(targetBuf)),
                   m.entries[0].points);
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
  if (m.msgType == MSG_TABLE || m.msgType == MSG_DISCOVERY) diffAndForward(m);
  else if (m.msgType == MSG_PLAYER_STATE) handlePlayerState(m);
  else if (m.msgType == MSG_NFC) handleNfc(m);
  else if (m.msgType == MSG_HIT_EVENT) handleHitEvent(m);
}

void espNowBegin() {
  if (espNowReady) return;
  if (esp_now_init() != ESP_OK) { LT_LOG("esp_now_init failed"); return; }
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcast, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  espNowReady = true;
  LT_LOG("ESP-NOW ready (channel %d)", WiFi.channel());
}

void espNowLoop() { }

void espNowBroadcastPhase() {
  if (!espNowReady) return;
  Message m = {};
  strlcpy(m.senderName, g_serverName, sizeof(m.senderName));
  m.msgType = MSG_PHASE;
  m.playerCount = 1;
  m.entries[0].playerId = (uint32_t)g_matchPhase;
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
  m.playerCount = 2;
  m.entries[1].points = g_settings.hitPoints;
  esp_now_send(kBroadcast, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
}

void espNowBroadcastTeams() {
  if (!espNowReady) return;
  if (g_settings.teamCount == 0) return;
  Message m = {};
  strlcpy(m.senderName, g_serverName, sizeof(m.senderName));
  m.msgType = MSG_TEAMS;
  m.playerCount = g_settings.teamCount;
  for (int i = 0; i < g_settings.teamCount && i < MAX_PLAYERS; i++) {
    const TeamDef &t = g_settings.teams[i];
    strlcpy(m.entries[i].player, t.name, sizeof(m.entries[i].player));
    m.entries[i].playerId = t.color;
    m.entries[i].lastUpdate = t.memberBits;
    m.entries[i].points = 0;
  }
  esp_now_send(kBroadcast, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
}

void espNowBroadcastPlayerConfig() {
  if (!espNowReady) return;
  Message m = {};
  strlcpy(m.senderName, g_serverName, sizeof(m.senderName));
  m.msgType = MSG_PLAYER_CONFIG;
  int count = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    PlayerSnapshot &s = g_snapshots[i];
    if (s.player[0] == '\0') continue;
    uint8_t cmd = necCommand(s.playerId);
    int teamIdx = (strcmp(g_settings.mode, "team") == 0) ? teamIndexForCommand(cmd) : count;
    if (teamIdx < 0) teamIdx = count;
    s.teamIndex = (uint8_t)teamIdx;
    if (strcmp(g_settings.mode, "team") == 0 && teamIdx < g_settings.teamCount) {
      s.color = g_settings.teams[teamIdx].color;
    } else if (s.color == 0) {
      s.color = defaultColorForPlayer(s.playerId, i);
    }
    m.entries[count].playerId = s.playerId;
    strlcpy(m.entries[count].player, s.player, sizeof(m.entries[count].player));
    m.entries[count].points = (int16_t)teamIdx;
    m.entries[count].lastUpdate = s.color & 0x00FFFFFFu;
    count++;
  }
  m.playerCount = count;
  esp_now_send(kBroadcast, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
}

int espNowGetRanking(PlayerSnapshot *out) {
  int count = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] != '\0') out[count++] = g_snapshots[i];
  }
  return count;
}
