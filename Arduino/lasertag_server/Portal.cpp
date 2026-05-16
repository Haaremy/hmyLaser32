#include "Portal.h"
#include <WebServer.h>
#include "Config.h"
#include "Globals.h"
#include "Identity.h"
#include "Match.h"
#include "Storage.h"
#include "WiFiSetup.h"

static WebServer server(80);

static bool checkPin(const String &given) {
  return given == String(g_serverPin);
}

// Hilfreich für Captive-Portal-Erkennung: Apple, Android und Windows
// erwarten konkrete URLs. Wir antworten mit Redirect auf /.
static void redirectToRoot() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

static void handleRoot() {
  String html;
  html.reserve(4500);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>hmyLaser32 Server</title>");
  html += F("<style>"
            "body{font-family:system-ui,sans-serif;background:#0f172a;color:#f8fafc;margin:0;padding:1.5rem;line-height:1.5}"
            "h1{margin:.2rem 0;font-size:1.4rem}h2{font-size:1.05rem;margin-top:1.4rem;border-bottom:1px solid #334155;padding-bottom:.3rem}"
            ".card{background:#1e293b;border:1px solid #334155;border-radius:.6rem;padding:1rem;margin-bottom:1rem}"
            "label{display:block;margin:.5rem 0 .2rem;font-size:.85rem;color:#cbd5e1}"
            "input,select,button{width:100%;padding:.55rem .7rem;border:1px solid #475569;border-radius:.4rem;background:#0f172a;color:#f8fafc;font:inherit;box-sizing:border-box}"
            "button{background:#2563eb;border-color:#2563eb;color:#fff;font-weight:600;cursor:pointer;margin-top:.6rem}"
            "button:hover{background:#1d4ed8}"
            ".pin{font-family:monospace;font-size:1.5rem;letter-spacing:.3em;background:#0f172a;padding:.4rem .8rem;border-radius:.4rem;display:inline-block}"
            ".net{display:flex;align-items:center;gap:.6rem;padding:.4rem 0;border-bottom:1px dashed #334155;cursor:pointer}"
            ".net:hover{color:#0ea5e9}"
            ".rssi{font-family:monospace;font-size:.75rem;color:#94a3b8;margin-left:auto}"
            ".lock{font-size:.7rem;color:#64748b}"
            ".pill{display:inline-block;padding:2px 8px;border-radius:999px;font-size:.7rem;font-weight:600;text-transform:uppercase}"
            ".ok{background:#16a34a;color:#fff}.warn{background:#d97706;color:#fff}.bad{background:#dc2626;color:#fff}"
            "small{color:#94a3b8}"
            "</style></head><body>");
  html += F("<h1>hmyLaser32 Server</h1>");
  html += "<div class='card'><div><strong>Name:</strong> <code>";
  html += g_serverName;
  html += "</code></div><div style='margin-top:.5rem'><strong>PIN:</strong> <span class='pin'>";
  html += g_serverPin;
  html += "</span></div><div style='margin-top:.6rem'><span class='pill ";
  html += (g_wifiConnected ? "ok" : "warn");
  html += "'>WLAN ";
  html += (g_wifiConnected ? "verbunden" : "Setup-Modus");
  html += "</span> <span class='pill ";
  html += (g_portalRegistered ? "ok" : "bad");
  html += "'>Portal ";
  html += (g_portalRegistered ? "registriert" : "offline");
  html += "</span> <span class='pill ";
  html += (g_matchPhase == MATCH_ACTIVE ? "ok" : g_matchPhase == MATCH_LOBBY ? "warn" : "bad");
  html += "'>Match ";
  html += (g_matchPhase == MATCH_ACTIVE ? "ACTIVE" : g_matchPhase == MATCH_LOBBY ? "LOBBY" : g_matchPhase == MATCH_DONE ? "DONE" : "IDLE");
  html += "</span></div></div>";

  html += F("<h2>WLAN-Setup</h2><div class='card'>");
  html += F("<div id='scan'><button onclick='scan()'>Verfügbare WLANs scannen</button>");
  html += F("<div id='netlist' style='margin-top:.7rem'></div></div>");
  html += F("<form method='POST' action='/api/wifi' onsubmit='return saveWifi(event)'>");
  html += F("<label>SSID</label><input name='ssid' id='ssid' required maxlength='32'>");
  html += F("<label>Passwort</label><input name='psk' id='psk' type='password' maxlength='64'>");
  html += F("<button type='submit'>WLAN speichern + Neustart</button></form></div>");

  html += F("<h2>Match-Einstellungen</h2><div class='card'>");
  html += F("<form method='POST' action='/api/settings'>");
  html += F("<label>Modus</label><input name='mode' value='");
  html += g_settings.mode;
  html += F("' maxlength='23'>");
  html += F("<label>Lobby-Sekunden (Verteilphase)</label><input name='lobby' type='number' min='5' max='600' value='");
  html += String((unsigned long)g_settings.lobbySeconds);
  html += F("'>");
  html += F("<label>Match-Sekunden</label><input name='match' type='number' min='30' max='3600' value='");
  html += String((unsigned long)g_settings.matchSeconds);
  html += F("'>");
  html += F("<label>PIN zum Bestätigen</label><input name='pin' type='password' maxlength='8' required>");
  html += F("<button type='submit'>Speichern</button></form></div>");

  html += F("<h2>Match starten</h2><div class='card'>");
  html += F("<form method='POST' action='/api/match/start'>");
  html += F("<label>PIN</label><input name='pin' type='password' maxlength='8' required>");
  html += F("<button type='submit'>Match starten (Lobby-Phase)</button></form>");
  html += F("<small>Nach der Lobby-Zeit beginnt die Spielphase automatisch.</small></div>");

  html += F("<h2>Identität zurücksetzen</h2><div class='card'>");
  html += F("<form method='POST' action='/api/identity/reset' onsubmit='return confirm(\"Identität zurücksetzen?\");'>");
  html += F("<label>PIN</label><input name='pin' type='password' maxlength='8' required>");
  html += F("<button type='submit' style='background:#dc2626;border-color:#dc2626'>Reset Name+PIN</button></form></div>");

  html += F("<script>"
            "function scan(){var d=document.getElementById('netlist');d.innerHTML='Scanne…';"
            "fetch('/api/scan').then(r=>r.json()).then(list=>{d.innerHTML='';list.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{"
            "var e=document.createElement('div');e.className='net';"
            "e.innerHTML='<span>'+n.ssid+'</span>'+(n.secure?'<span class=lock>🔒</span>':'')+'<span class=rssi>'+n.rssi+' dBm</span>';"
            "e.onclick=()=>{document.getElementById('ssid').value=n.ssid;document.getElementById('psk').focus()};d.appendChild(e)});});}"
            "function saveWifi(e){e.preventDefault();var f=new FormData(e.target);"
            "fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(f)})"
            ".then(r=>r.text()).then(t=>{alert('Gespeichert. Gerät startet neu.');});return false}"
            "</script>");
  html += F("</body></html>");
  server.send(200, "text/html", html);
}

static void handleScan() {
  server.send(200, "application/json", wifiScanJson());
}

static void handleWifiSave() {
  String ssid = server.arg("ssid");
  String psk  = server.arg("psk");
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "ssid required");
    return;
  }
  storageSetString("ssid", ssid);
  storageSetString("psk",  psk);
  server.send(200, "text/plain", "ok, restarting...");
  delay(800);
  ESP.restart();
}

static void handleSettingsGet() {
  String j = "{\"name\":\"";
  j += g_serverName;
  j += "\",\"mode\":\"";
  j += g_settings.mode;
  j += "\",\"lobbySeconds\":";
  j += String((unsigned long)g_settings.lobbySeconds);
  j += ",\"matchSeconds\":";
  j += String((unsigned long)g_settings.matchSeconds);
  j += ",\"wifiConnected\":";  j += (g_wifiConnected ? "true" : "false");
  j += ",\"portalRegistered\":"; j += (g_portalRegistered ? "true" : "false");
  j += ",\"matchPhase\":";    j += String((int)g_matchPhase);
  j += "}";
  server.send(200, "application/json", j);
}

static void handleSettingsPost() {
  if (!checkPin(server.arg("pin"))) {
    server.send(403, "text/plain", "invalid pin");
    return;
  }
  if (server.hasArg("mode"))  strlcpy(g_settings.mode, server.arg("mode").c_str(), sizeof(g_settings.mode));
  if (server.hasArg("lobby")) g_settings.lobbySeconds = (uint32_t)server.arg("lobby").toInt();
  if (server.hasArg("match")) g_settings.matchSeconds = (uint32_t)server.arg("match").toInt();
  matchSaveSettings();
  redirectToRoot();
}

static void handleMatchStart() {
  if (!checkPin(server.arg("pin"))) {
    server.send(403, "text/plain", "invalid pin");
    return;
  }
  bool ok = matchStart();
  if (!ok) { server.send(409, "text/plain", "already running"); return; }
  redirectToRoot();
}

static void handleIdentityReset() {
  if (!checkPin(server.arg("pin"))) {
    server.send(403, "text/plain", "invalid pin");
    return;
  }
  identityRegenerate();
  // Cloud-Re-Register passiert beim nächsten Heartbeat-Versuch
  redirectToRoot();
}

static void handleStatus() {
  String j = "{\"name\":\"";
  j += g_serverName;
  j += "\",\"pin\":\"";
  j += g_serverPin;
  j += "\",\"wifi\":";       j += (g_wifiConnected ? "true" : "false");
  j += ",\"registered\":";   j += (g_portalRegistered ? "true" : "false");
  j += ",\"matchPhase\":";   j += String((int)g_matchPhase);
  if (g_matchPhase == MATCH_LOBBY) {
    long left = (long)(g_lobbyEndsAtMs - millis()) / 1000;
    j += ",\"lobbyLeft\":";  j += String(left);
  }
  if (g_matchPhase == MATCH_ACTIVE) {
    long left = (long)(g_matchEndsAtMs - millis()) / 1000;
    j += ",\"matchLeft\":";  j += String(left);
  }
  j += "}";
  server.send(200, "application/json", j);
}

void portalBegin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);
  server.on("/api/settings", HTTP_GET, handleSettingsGet);
  server.on("/api/settings", HTTP_POST, handleSettingsPost);
  server.on("/api/match/start", HTTP_POST, handleMatchStart);
  server.on("/api/identity/reset", HTTP_POST, handleIdentityReset);
  server.on("/api/status", HTTP_GET, handleStatus);

  // Captive-Portal-Endpoints (OS-spezifisch)
  server.on("/generate_204", HTTP_GET, redirectToRoot);          // Android
  server.on("/hotspot-detect.html", HTTP_GET, redirectToRoot);   // iOS / macOS
  server.on("/ncsi.txt", HTTP_GET, redirectToRoot);              // Windows
  server.onNotFound(redirectToRoot);

  server.begin();
  LT_LOG("HTTP server up on :80");
}

void portalLoop() {
  server.handleClient();
}
