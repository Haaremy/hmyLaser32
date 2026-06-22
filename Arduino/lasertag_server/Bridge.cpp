#include "Bridge.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <cstring>
#include "Config.h"
#include "Globals.h"
#include "Match.h"

static WiFiClientSecure makeClient() {
  WiFiClientSecure c;
  c.setInsecure();
  return c;
}

static int postJson(const char *path, const String &body, String &response) {
  if (!g_wifiConnected) return -1;
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  String url = String(PORTAL_BASE_URL) + path;
  if (!http.begin(client, url)) return -2;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + g_serverPin);
  int code = http.POST(body);
  response = http.getString();
  http.end();
  return code;
}

static int getAuthed(const char *path, String &response) {
  if (!g_wifiConnected) return -1;
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  String url = String(PORTAL_BASE_URL) + path;
  if (!http.begin(client, url)) return -2;
  http.addHeader("Authorization", String("Bearer ") + g_serverPin);
  int code = http.GET();
  response = http.getString();
  http.end();
  return code;
}

static String jsonStringField(const String &json, const char *key) {
  String needle = String("\"") + key + "\":\"";
  int idx = json.indexOf(needle);
  if (idx < 0) return "";
  int start = idx + needle.length();
  int end = json.indexOf("\"", start);
  if (end < start) return "";
  return json.substring(start, end);
}

static long jsonNumberField(const String &json, const char *key, long fallback) {
  String needle = String("\"") + key + "\":";
  int idx = json.indexOf(needle);
  if (idx < 0) return fallback;
  int start = idx + needle.length();
  while (start < (int)json.length() && (json.charAt(start) == ' ' || json.charAt(start) == '"')) start++;
  long val = 0;
  bool any = false;
  for (int i = start; i < (int)json.length(); i++) {
    char c = json.charAt(i);
    if (c >= '0' && c <= '9') { val = val * 10 + (c - '0'); any = true; }
    else break;
  }
  return any ? val : fallback;
}

static bool jsonBoolField(const String &json, const char *key, bool fallback) {
  String needle = String("\"") + key + "\":";
  int idx = json.indexOf(needle);
  if (idx < 0) return fallback;
  int start = idx + needle.length();
  while (start < (int)json.length() && (json.charAt(start) == ' ' || json.charAt(start) == '"')) start++;
  if (json.substring(start, start + 4) == "true") return true;
  if (json.substring(start, start + 5) == "false") return false;
  return fallback;
}

static int findMatchingBracket(const String &s, int openPos, char openChar, char closeChar) {
  if (openPos < 0 || openPos >= (int)s.length() || s.charAt(openPos) != openChar) return -1;
  int depth = 0;
  bool inString = false;
  bool escaped = false;
  for (int i = openPos; i < (int)s.length(); i++) {
    char c = s.charAt(i);
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        inString = false;
      }
      continue;
    }
    if (c == '"') {
      inString = true;
    } else if (c == openChar) {
      depth++;
    } else if (c == closeChar) {
      depth--;
      if (depth == 0) return i;
    }
  }
  return -1;
}

// Parse the "teams":[...] array minimal:
// erwartet `[{"name":"...","color":"#rrggbb","members":[1,2,3]}, ...]`.
static void parseTeams(const String &json, TeamDef *teams, uint8_t &outCount) {
  outCount = 0;
  int teamsStart = json.indexOf("\"teams\":");
  if (teamsStart < 0) return;
  int arrStart = json.indexOf('[', teamsStart);
  int arrEnd = findMatchingBracket(json, arrStart, '[', ']');
  if (arrStart < 0 || arrEnd < 0 || arrEnd <= arrStart) return;

  int i = arrStart + 1;
  while (i < arrEnd && outCount < MAX_TEAMS) {
    int objStart = json.indexOf('{', i);
    if (objStart < 0 || objStart >= arrEnd) break;
    int objEnd = findMatchingBracket(json, objStart, '{', '}');
    if (objEnd < 0 || objEnd > arrEnd) break;
    String obj = json.substring(objStart, objEnd + 1);

    TeamDef td = {};
    String name = jsonStringField(obj, "name");
    strlcpy(td.name, name.c_str(), sizeof(td.name));

    String color = jsonStringField(obj, "color");
    // "#rrggbb" → 0x00rrggbb
    if (color.length() == 7 && color.charAt(0) == '#') {
      td.color = (uint32_t)strtoul(color.substring(1).c_str(), nullptr, 16) & 0x00FFFFFFu;
    } else {
      td.color = 0x808080;
    }

    // members:[n,n,...]
    int membersStart = obj.indexOf("\"members\":");
    if (membersStart >= 0) {
      int mArrStart = obj.indexOf('[', membersStart);
      int mArrEnd   = obj.indexOf(']', mArrStart);
      if (mArrStart >= 0 && mArrEnd > mArrStart) {
        String list = obj.substring(mArrStart + 1, mArrEnd);
        int j = 0;
        while (j < (int)list.length()) {
          while (j < (int)list.length() && (list.charAt(j) == ' ' || list.charAt(j) == ',')) j++;
          int num = 0;
          bool any = false;
          while (j < (int)list.length() && list.charAt(j) >= '0' && list.charAt(j) <= '9') {
            num = num * 10 + (list.charAt(j) - '0');
            any = true;
            j++;
          }
          if (any && num >= 1 && num <= 32) {
            td.memberBits |= (1u << (num - 1));
          }
        }
      }
    }

    teams[outCount++] = td;
    i = objEnd + 1;
  }
}

bool bridgeRegister() {
  if (!g_wifiConnected) return false;
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  String url = String(PORTAL_BASE_URL) + EP_REGISTER;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  String body = "{\"name\":\"";
  body += g_serverName;
  body += "\",\"pin\":\"";
  body += g_serverPin;
  body += "\"}";
  int code = http.POST(body);
  String resp = http.getString();
  http.end();
  LT_LOG("register HTTP %d", code);
  if (code == 200 || code == 201) {
    String id = jsonStringField(resp, "id");
    if (id.length() > 0) strlcpy(g_serverId, id.c_str(), sizeof(g_serverId));
    g_portalRegistered = true;
    return true;
  }
  return false;
}

bool bridgeHeartbeat() {
  String resp;
  int code = getAuthed(EP_REGISTER, resp);
  if (code == 200) return true;
  if (code == 401) { g_portalRegistered = false; return bridgeRegister(); }
  return false;
}

bool bridgePullSettings(bool &out_startRequested) {
  out_startRequested = false;
  String resp;
  int code = getAuthed(EP_ESP_SETTINGS, resp);
  if (code != 200) return false;

  String mode = jsonStringField(resp, "mode");
  long lobby = jsonNumberField(resp, "lobbySeconds", -1);
  long match = jsonNumberField(resp, "matchSeconds", -1);
  bool startReq = jsonBoolField(resp, "startRequested", false);
  long hitPoints = jsonNumberField(resp, "hitPoints", -1);
  long zone1Points = jsonNumberField(resp, "zone1Points", -1);
  long zone2Points = jsonNumberField(resp, "zone2Points", -1);
  long zone3Points = jsonNumberField(resp, "zone3Points", -1);

  bool changed = false;
  if (mode.length() > 0 && mode != String(g_settings.mode)) {
    strlcpy(g_settings.mode, mode.c_str(), sizeof(g_settings.mode));
    changed = true;
  }
  if (lobby > 0 && (uint32_t)lobby != g_settings.lobbySeconds) { g_settings.lobbySeconds = (uint32_t)lobby; changed = true; }
  if (match > 0 && (uint32_t)match != g_settings.matchSeconds) { g_settings.matchSeconds = (uint32_t)match; changed = true; }
  if (hitPoints > 0 && (int16_t)hitPoints != g_settings.hitPoints) { g_settings.hitPoints = (int16_t)hitPoints; changed = true; }
  if (zone1Points > 0 && (int16_t)zone1Points != g_settings.zonePoints[0]) { g_settings.zonePoints[0] = (int16_t)zone1Points; changed = true; }
  if (zone2Points > 0 && (int16_t)zone2Points != g_settings.zonePoints[1]) { g_settings.zonePoints[1] = (int16_t)zone2Points; changed = true; }
  if (zone3Points > 0 && (int16_t)zone3Points != g_settings.zonePoints[2]) { g_settings.zonePoints[2] = (int16_t)zone3Points; changed = true; }
  g_settings.hitPoints = g_settings.zonePoints[0];

  // Teams parsen — wir überschreiben immer, weil das JSON die Source of Truth ist
  TeamDef newTeams[MAX_TEAMS] = {};
  uint8_t newCount = 0;
  parseTeams(resp, newTeams, newCount);

  bool teamsChanged = (newCount != g_settings.teamCount) ||
                      memcmp(newTeams, g_settings.teams, sizeof(newTeams)) != 0;
  if (teamsChanged) {
    memcpy(g_settings.teams, newTeams, sizeof(g_settings.teams));
    g_settings.teamCount = newCount;
    changed = true;
    LT_LOG("Teams updated: %u defined", (unsigned)newCount);
  }
  if (changed) matchSaveSettings();
  out_startRequested = startReq;
  return true;
}

bool bridgeStartMatch() {
  String body = "{\"mode\":\"";
  body += g_settings.mode;
  body += "\",\"durationSeconds\":";
  body += String((unsigned long)g_settings.matchSeconds);
  body += ",\"hitPoints\":";
  body += String((int)g_settings.zonePoints[0]);
  body += ",\"zone1Points\":";
  body += String((int)g_settings.zonePoints[0]);
  body += ",\"zone2Points\":";
  body += String((int)g_settings.zonePoints[1]);
  body += ",\"zone3Points\":";
  body += String((int)g_settings.zonePoints[2]);

  if (g_settings.teamCount > 0) {
    body += ",\"teams\":[";
    for (uint8_t i = 0; i < g_settings.teamCount; i++) {
      if (i) body += ",";
      char hex[8];
      snprintf(hex, sizeof(hex), "#%06lx", (unsigned long)(g_settings.teams[i].color & 0x00FFFFFFu));
      body += "{\"name\":\"";
      body += g_settings.teams[i].name;
      body += "\",\"color\":\"";
      body += hex;
      body += "\"}";
    }
    body += "]";
  }
  body += "}";

  String resp;
  int code = postJson(EP_MATCH_START, body, resp);
  LT_LOG("match/start HTTP %d", code);
  if (code == 200 || code == 201) {
    String mid = jsonStringField(resp, "matchId");
    if (mid.length() > 0) {
      strlcpy(g_currentMatchId, mid.c_str(), sizeof(g_currentMatchId));
      return true;
    }
  }
  return false;
}

bool bridgeForwardHit(const char *shooterNfc, const char *targetNfc, int points) {
  if (strlen(g_currentMatchId) == 0) return false;
  String body = "{\"matchId\":\"";
  body += g_currentMatchId;
  body += "\",\"shooterNfc\":\"";
  body += shooterNfc;
  body += "\",\"targetNfc\":\"";
  body += targetNfc;
  body += "\",\"points\":";
  body += String(points);
  body += "}";
  String resp;
  int code = postJson(EP_HIT, body, resp);
  if (code != 200) LT_LOG("hit HTTP %d", code);
  return code == 200;
}

static uint8_t reverse8(uint8_t value) {
  value = (value & 0xF0) >> 4 | (value & 0x0F) << 4;
  value = (value & 0xCC) >> 2 | (value & 0x33) << 2;
  value = (value & 0xAA) >> 1 | (value & 0x55) << 1;
  return value;
}

// Extrahiert die urspruengliche NEC-Command-ID aus dem 32-bit playerId.
// IRremoteESP8266::encodeNEC legt das Command-Byte bit-reversed ab.
static uint8_t necCommand(uint32_t playerId) {
  return reverse8((uint8_t)((playerId >> 8) & 0xFFu));
}

bool bridgePushKnownPlayers() {
  if (!g_wifiConnected || !g_portalRegistered) return false;

  // Body aus g_snapshots zusammensetzen
  String body = "{\"players\":[";
  bool first = true;
  unsigned long nowMs = millis();
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] == '\0') continue;
    uint8_t cmd = necCommand(g_snapshots[i].playerId);
    if (cmd == 0) continue;
    if (!first) body += ",";
    first = false;
    body += "{\"name\":\"";
    body += g_snapshots[i].player;
    body += "\",\"command\":";
    body += String((unsigned)cmd);
    body += ",\"playerId\":";
    body += String((unsigned long)g_snapshots[i].playerId);
    body += ",\"points\":";
    body += String((int)g_snapshots[i].lastPoints);
    body += ",\"shots\":";
    body += String((unsigned)g_snapshots[i].shotsFired);
    body += ",\"rxHits\":";
    body += String((unsigned)g_snapshots[i].rxHits);
    body += ",\"team\":";
    body += String((unsigned)g_snapshots[i].teamIndex + 1);
    char color[8];
    snprintf(color, sizeof(color), "#%06lx", (unsigned long)(g_snapshots[i].color & 0x00FFFFFFu));
    body += ",\"color\":\"";
    body += color;
    body += "\"";
    if (g_snapshots[i].nfcToken[0] != '\0') {
      body += ",\"nfcToken\":\"";
      body += g_snapshots[i].nfcToken;
      body += "\"";
    }
    body += ",\"lastSeenSec\":";
    body += String((unsigned long)((nowMs - g_snapshots[i].lastUpdate) / 1000UL));
    body += "}";
  }
  body += "]}";

  String resp;
  int code = postJson(EP_PLAYERS, body, resp);
  if (code != 200) LT_LOG("players push HTTP %d", code);
  return code == 200;
}

bool bridgeEndMatch(const EndPlayer *players, int count) {
  if (strlen(g_currentMatchId) == 0) return false;
  String body = "{\"matchId\":\"";
  body += g_currentMatchId;
  body += "\",\"players\":[";
  for (int i = 0; i < count; i++) {
    if (i) body += ",";
    body += "{\"nfcToken\":\"";
    body += players[i].nfc;
    body += "\"";
    if (players[i].team) { body += ",\"teamName\":\""; body += players[i].team; body += "\""; }
    body += ",\"hits\":";   body += String(players[i].hits);
    body += ",\"deaths\":"; body += String(players[i].deaths);
    body += ",\"points\":"; body += String(players[i].points);
    body += "}";
  }
  body += "]}";
  String resp;
  int code = postJson(EP_MATCH_END, body, resp);
  LT_LOG("match/end HTTP %d", code);
  return code == 200;
}
