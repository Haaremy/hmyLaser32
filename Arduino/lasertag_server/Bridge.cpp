#include "Bridge.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Config.h"
#include "Globals.h"

// In Produktion sollte hier ein gepinntes Root-CA-Cert hinterlegt werden.
// Für den Erstwurf nutzen wir `setInsecure()` — TLS-Zertifikatsvalidierung
// aus. Sicherheitsrelevant nur das hmyLaser32-Portal, kein Auth-Header
// verlässt das Gerät außer dem PIN.
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

bool bridgeRegister() {
  if (!g_wifiConnected) return false;

  // POST /api/bridge/register (KEIN Bearer beim Initial-Register — der ESP
  // sendet name+pin, der Server akzeptiert. Wir nutzen einen extra Pfad
  // ohne Authorization.)
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

  LT_LOG("register HTTP %d: %s", code, resp.c_str());

  if (code == 200 || code == 201) {
    // matchId/id aus JSON ziehen — billig ohne JSON-Lib
    int idIdx = resp.indexOf("\"id\":\"");
    if (idIdx >= 0) {
      int start = idIdx + 6;
      int end = resp.indexOf("\"", start);
      if (end > start) {
        String id = resp.substring(start, end);
        strlcpy(g_serverId, id.c_str(), sizeof(g_serverId));
      }
    }
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
    // PIN nicht (mehr) gültig → erneut registrieren
    g_portalRegistered = false;
    return bridgeRegister();
  }
  return false;
}

bool bridgeStartMatch() {
  String body = "{\"mode\":\"";
  body += g_settings.mode;
  body += "\",\"durationSeconds\":";
  body += String((unsigned long)g_settings.matchSeconds);
  body += "}";
  String resp;
  int code = postJson(EP_MATCH_START, body, resp);
  LT_LOG("match/start HTTP %d: %s", code, resp.c_str());
  if (code == 200 || code == 201) {
    int idx = resp.indexOf("\"matchId\":\"");
    if (idx >= 0) {
      int start = idx + 11;
      int end = resp.indexOf("\"", start);
      if (end > start) {
        String mid = resp.substring(start, end);
        strlcpy(g_currentMatchId, mid.c_str(), sizeof(g_currentMatchId));
        return true;
      }
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
  if (code != 200) LT_LOG("hit HTTP %d: %s", code, resp.c_str());
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
    if (players[i].team) {
      body += ",\"teamName\":\"";
      body += players[i].team;
      body += "\"";
    }
    body += ",\"hits\":";    body += String(players[i].hits);
    body += ",\"deaths\":";  body += String(players[i].deaths);
    body += ",\"points\":";  body += String(players[i].points);
    body += "}";
  }
  body += "]}";
  String resp;
  int code = postJson(EP_MATCH_END, body, resp);
  LT_LOG("match/end HTTP %d: %s", code, resp.c_str());
  return code == 200;
}
