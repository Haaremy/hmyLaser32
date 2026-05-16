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

// Parse the "teams":[...] array minimal:
// erwartet `[{"name":"...","color":"#rrggbb","members":[1,2,3]}, ...]`.
static void parseTeams(const String &json, TeamDef *teams, uint8_t &outCount) {
  outCount = 0;
  int teamsStart = json.indexOf("\"teams\":");
  if (teamsStart < 0) return;
  int arrStart = json.indexOf('[', teamsStart);
  int arrEnd = json.indexOf(']', arrStart);
  if (arrStart < 0 || arrEnd < 0 || arrEnd <= arrStart) return;

  int i = arrStart + 1;
  while (i < arrEnd && outCount < MAX_TEAMS) {
    int objStart = json.indexOf('{', i);
    if (objStart < 0 || objStart >= arrEnd) break;
    int objEnd = json.indexOf('}', objStart);
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

  bool changed = false;
  if (mode.length() > 0 && mode != String(g_settings.mode)) {
    strlcpy(g_settings.mode, mode.c_str(), sizeof(g_settings.mode));
    changed = true;
  }
  if (lobby > 0 && (uint32_t)lobby != g_settings.lobbySeconds) { g_settings.lobbySeconds = (uint32_t)lobby; changed = true; }
  if (match > 0 && (uint32_t)match != g_settings.matchSeconds) { g_settings.matchSeconds = (uint32_t)match; changed = true; }

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
