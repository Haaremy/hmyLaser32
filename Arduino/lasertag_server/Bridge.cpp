#include "Bridge.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Config.h"
#include "Globals.h"
#include "Match.h"

// Insecure TLS für v1; Pinning siehe TODO im README.
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

bool bridgeRegister() {
  if (!g_wifiConnected) return false;
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  String url = String(PORTAL_BASE_URL) + EP_REGISTER;
  if (!http.begin(client, url)) {
    LT_LOG("http.begin failed");
    return false;
  }
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
  if (code == 401) {
    g_portalRegistered = false;
    return bridgeRegister();
  }
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
  if (lobby > 0 && (uint32_t)lobby != g_settings.lobbySeconds) {
    g_settings.lobbySeconds = (uint32_t)lobby;
    changed = true;
  }
  if (match > 0 && (uint32_t)match != g_settings.matchSeconds) {
    g_settings.matchSeconds = (uint32_t)match;
    changed = true;
  }
  if (changed) {
    matchSaveSettings();
    LT_LOG("Settings updated from webservice");
  }
  out_startRequested = startReq;
  return true;
}

bool bridgeStartMatch() {
  String body = "{\"mode\":\"";
  body += g_settings.mode;
  body += "\",\"durationSeconds\":";
  body += String((unsigned long)g_settings.matchSeconds);
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
