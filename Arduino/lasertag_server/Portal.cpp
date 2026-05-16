#include "Portal.h"
#include <WebServer.h>
#include "Config.h"
#include "Globals.h"
#include "Identity.h"
#include "Match.h"
#include "Storage.h"
#include "WiFiSetup.h"

static WebServer server(80);

static bool checkPin(const String &given) { return given == String(g_serverPin); }

static void redirectToRoot() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

static const char *phaseName(MatchPhase p) {
  switch (p) {
    case MATCH_LOBBY:  return "LOBBY";
    case MATCH_ACTIVE: return "ACTIVE";
    case MATCH_DONE:   return "DONE";
    default:           return "IDLE";
  }
}

static void handleRoot() {
  String html;
  html.reserve(5500);
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
            "button:hover{background:#1d4ed8}button.warn{background:#d97706;border-color:#d97706}button.danger{background:#dc2626;border-color:#dc2626}"
            ".pin{font-family:monospace;font-size:1.5rem;letter-spacing:.3em;background:#0f172a;padding:.4rem .8rem;border-radius:.4rem;display:inline-block}"
            ".net{display:flex;align-items:center;gap:.6rem;padding:.4rem 0;border-bottom:1px dashed #334155;cursor:pointer}"
            ".net:hover{color:#0ea5e9}"
            ".rssi{font-family:monospace;font-size:.75rem;color:#94a3b8;margin-left:auto}"
            ".lock{font-size:.7rem;color:#64748b}"
            ".pill{display:inline-block;padding:2px 8px;border-radius:999px;font-size:.7rem;font-weight:600;text-transform:uppercase}"
            ".ok{background:#16a34a;color:#fff}.warn{background:#d97706;color:#fff}.bad{background:#dc2626;color:#fff}"
            ".kv{display:grid;grid-template-columns:120px 1fr;gap:.4rem;margin:.3rem 0;font-size:.85rem}"
            ".kv .k{color:#94a3b8}.kv .v{font-family:monospace;color:#f8fafc;word-break:break-all}"
            "small{color:#94a3b8}"
            "</style></head><body>");
  html += F("<h1>hmyLaser32 Server</h1>");
  // Top-Card mit Identität + Endpoints
  html += "<div class='card'><div><strong>Name:</strong> <code>";
  html += g_serverName;
  html += "</code></div><div style='margin-top:.5rem'><strong>PIN:</strong> <span class='pin'>";
  html += g_serverPin;
  html += "</span></div>";

  html += "<div class='kv' style='margin-top:.8rem'>";
  html += "<div class='k'>AP-SSID</div><div class='v'>";  html += g_apSsid;            html += " (offen)</div>";
  html += "<div class='k'>AP-IP</div><div class='v'>";    html += "192.168.4.1";        html += "</div>";
  html += "<div class='k'>WLAN-Status</div><div class='v'>"; html += (g_wifiConnected ? "verbunden" : "Setup-Mode"); html += "</div>";
  if (g_wifiConnected) {
    html += "<div class='k'>LAN-IP</div><div class='v'>";  html += g_staIp;             html += "</div>";
    html += "<div class='k'>mDNS</div><div class='v'>";    html += (g_mdnsHost[0] ? String("http://") + g_mdnsHost + ".local/" : "(inaktiv)"); html += "</div>";
    html += "<div class='k'>WLAN-Channel</div><div class='v'>"; html += String((int)g_staChannel); html += "</div>";
  }
  html += "<div class='k'>Portal</div><div class='v'>"; html += (g_portalRegistered ? "registriert" : "offline"); html += "</div>";
  html += "<div class='k'>Match-Phase</div><div class='v'>"; html += phaseName(g_matchPhase); html += "</div>";
  html += "</div></div>";

  // WLAN-Setup
  html += F("<h2>WLAN-Setup</h2><div class='card'>");
  html += F("<div id='scan'><button onclick='scan()' type='button'>Verfügbare WLANs scannen</button>");
  html += F("<div id='netlist' style='margin-top:.7rem'></div></div>");
  html += F("<form method='POST' action='/api/wifi' onsubmit='return saveWifi(event)'>");
  html += F("<label>SSID</label><input name='ssid' id='ssid' required maxlength='32'>");
  html += F("<label>Passwort</label><input name='psk' id='psk' type='password' maxlength='64'>");
  html += F("<button type='submit'>WLAN speichern + Neustart</button></form>");
  if (g_wifiConnected) {
    html += F("<form method='POST' action='/api/wifi/forget' style='margin-top:.6rem' onsubmit='return confirm(\"WLAN-Login löschen?\");'>");
    html += F("<label>PIN</label><input name='pin' type='password' maxlength='8' required>");
    html += F("<button type='submit' class='warn'>WLAN-Login löschen (zurück in AP-only)</button></form>");
  }
  html += F("</div>");

  // Match-Settings
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
  html += F("<button type='submit'>Speichern</button></form>");
  html += F("<small>Diese Werte werden auch vom Webservice gepullt — Änderungen dort wirken sich nach ~30 s aus.</small></div>");

  // Match steuern
  html += F("<h2>Match steuern</h2><div class='card'>");
  if (g_matchPhase == MATCH_IDLE || g_matchPhase == MATCH_DONE) {
    html += F("<form method='POST' action='/api/match/start'>");
    html += F("<label>PIN</label><input name='pin' type='password' maxlength='8' required>");
    html += F("<button type='submit'>Match starten (Lobby-Phase)</button></form>");
  } else {
    html += F("<form method='POST' action='/api/match/abort' onsubmit='return confirm(\"Match abbrechen?\");'>");
    html += F("<label>PIN</label><input name='pin' type='password' maxlength='8' required>");
    html += F("<button type='submit' class='danger'>Match abbrechen</button></form>");
  }
  html += F("</div>");

  // Identitäts-Reset
  html += F("<h2>Identität</h2><div class='card'>");
  html += F("<form method='POST' action='/api/identity/reset' onsubmit='return confirm(\"Name + PIN neu generieren?\");'>");
  html += F("<label>PIN</label><input name='pin' type='password' maxlength='8' required>");
  html += F("<button type='submit' class='danger'>Reset Name + PIN</button></form></div>");

  html += F("<script>"
            "function scan(){var d=document.getElementById('netlist');d.innerHTML='Scanne…';"
            "fetch('/api/scan').then(r=>r.json()).then(list=>{d.innerHTML='';list.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{"
            "var e=document.createElement('div');e.className='net';"
            "e.innerHTML='<span>'+n.ssid+' <small>(Ch '+(n.channel||'?')+')</small></span>'+(n.secure?'<span class=lock>🔒</span>':'')+'<span class=rssi>'+n.rssi+' dBm</span>';"
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

static void handleWifiForget() {
  if (!checkPin(server.arg("pin"))) {
    server.send(403, "text/plain", "invalid pin");
    return;
  }
  wifiForgetCredentials();
  server.send(200, "text/plain", "wifi credentials cleared, restarting...");
  delay(500);
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
  j += ",\"matchPhase\":\"";   j += phaseName(g_matchPhase); j += "\"";
  j += ",\"apSsid\":\"";       j += g_apSsid;       j += "\"";
  j += ",\"staIp\":\"";        j += g_staIp;        j += "\"";
  j += ",\"mdns\":\"";         j += g_mdnsHost;     j += "\"";
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
  if (!checkPin(server.arg("pin"))) { server.send(403, "text/plain", "invalid pin"); return; }
  if (!matchStart()) { server.send(409, "text/plain", "already running"); return; }
  redirectToRoot();
}

static void handleMatchAbort() {
  if (!checkPin(server.arg("pin"))) { server.send(403, "text/plain", "invalid pin"); return; }
  matchAbort();
  redirectToRoot();
}

static void handleIdentityReset() {
  if (!checkPin(server.arg("pin"))) { server.send(403, "text/plain", "invalid pin"); return; }
  identityRegenerate();
  redirectToRoot();
}

static void handleStatus() {
  handleSettingsGet();
}

void portalBegin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);
  server.on("/api/wifi/forget", HTTP_POST, handleWifiForget);
  server.on("/api/settings", HTTP_GET, handleSettingsGet);
  server.on("/api/settings", HTTP_POST, handleSettingsPost);
  server.on("/api/match/start", HTTP_POST, handleMatchStart);
  server.on("/api/match/abort", HTTP_POST, handleMatchAbort);
  server.on("/api/identity/reset", HTTP_POST, handleIdentityReset);
  server.on("/api/status", HTTP_GET, handleStatus);

  // Captive-Portal-OS-Endpoints
  server.on("/generate_204",        HTTP_GET, redirectToRoot);
  server.on("/hotspot-detect.html", HTTP_GET, redirectToRoot);
  server.on("/ncsi.txt",            HTTP_GET, redirectToRoot);
  server.onNotFound(redirectToRoot);

  server.begin();
  LT_LOG("HTTP server up on :80");
}

void portalLoop() {
  server.handleClient();
}
