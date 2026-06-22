#include "Portal.h"
#include <WebServer.h>
#include <cstring>
#include <cstdlib>
#include "Config.h"
#include "EspNow.h"
#include "Globals.h"
#include "Identity.h"
#include "Match.h"
#include "Storage.h"
#include "WiFiSetup.h"

static WebServer server(80);

static void redirectToRoot() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

static const char *phaseName(MatchPhase p) {
  switch (p) {
    case MATCH_LOBBY:  return "LOBBY";
    case MATCH_DISTRIBUTING: return "DISTRIBUTING";
    case MATCH_ACTIVE: return "ACTIVE";
    case MATCH_DONE:   return "DONE";
    default:           return "IDLE";
  }
}

static const uint32_t UI_COLORS[10] = {
  0xdc2626, 0x2563eb, 0x16a34a, 0xd97706, 0x9333ea,
  0x0891b2, 0xeab308, 0xec4899, 0x64748b, 0xffffff
};

static void colorHex(uint32_t color, char *out, size_t len) {
  snprintf(out, len, "#%06lx", (unsigned long)(color & 0x00FFFFFFu));
}

static uint32_t parseColor(const String &s, uint32_t fallback) {
  if (s.length() == 7 && s.charAt(0) == '#') {
    return (uint32_t)strtoul(s.substring(1).c_str(), nullptr, 16) & 0x00FFFFFFu;
  }
  return fallback;
}

static uint8_t reverse8(uint8_t value) {
  value = (value & 0xF0) >> 4 | (value & 0x0F) << 4;
  value = (value & 0xCC) >> 2 | (value & 0x33) << 2;
  value = (value & 0xAA) >> 1 | (value & 0x55) << 1;
  return value;
}

static uint8_t commandFromPlayerId(uint32_t playerId) {
  return reverse8((uint8_t)((playerId >> 8) & 0xFFu));
}

static void rebuildTeamsFromSnapshots() {
  memset(g_settings.teams, 0, sizeof(g_settings.teams));
  uint8_t maxTeam = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    PlayerSnapshot &p = g_snapshots[i];
    if (p.player[0] == '\0') continue;
    uint8_t team = p.teamIndex < MAX_TEAMS ? p.teamIndex : 0;
    if (team + 1 > maxTeam) maxTeam = team + 1;
    TeamDef &t = g_settings.teams[team];
    if (t.name[0] == '\0') snprintf(t.name, sizeof(t.name), "Team %u", (unsigned)(team + 1));
    if (t.color == 0) t.color = p.color ? p.color : UI_COLORS[team % 10];
    uint8_t cmd = commandFromPlayerId(p.playerId);
    if (cmd >= 1 && cmd <= 32) t.memberBits |= (1u << (cmd - 1));
  }
  g_settings.teamCount = strcmp(g_settings.mode, "team") == 0 ? maxTeam : 0;
}

static void appendServerInfo(String &html) {
  html += "<div class='card'><div><strong>Name:</strong> <code>";
  html += g_serverName;
  html += "</code></div><div style='margin-top:.5rem'><strong>PIN:</strong> <span class='pin'>";
  html += g_serverPin;
  html += "</span></div><div class='kv' style='margin-top:.8rem'>";
  html += "<div class='k'>AP-SSID</div><div class='v'>"; html += g_apSsid; html += " (offen)</div>";
  html += "<div class='k'>AP-IP</div><div class='v'>192.168.4.1</div>";
  html += "<div class='k'>WLAN-Status</div><div class='v'>"; html += (g_wifiConnected ? "verbunden" : "Setup-Mode"); html += "</div>";
  if (g_wifiConnected) {
    html += "<div class='k'>LAN-IP</div><div class='v'>"; html += g_staIp; html += "</div>";
    html += "<div class='k'>mDNS</div><div class='v'>";
    html += (g_mdnsHost[0] ? String("http://") + g_mdnsHost + ".local/" : "(inaktiv)");
    html += "</div><div class='k'>WLAN-Channel</div><div class='v'>";
    html += String((int)g_staChannel);
    html += "</div>";
  }
  html += "<div class='k'>Portal</div><div class='v'>"; html += (g_portalRegistered ? "registriert" : "offline"); html += "</div>";
  html += "<div class='k'>Match-Phase</div><div class='v'>"; html += phaseName(g_matchPhase); html += "</div>";
  html += "</div></div>";
}

static void handleRoot() {
  String html;
  html.reserve(16000);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>hmyLaser32 Server</title><style>"
            "body{font-family:system-ui,sans-serif;background:#111827;color:#f9fafb;margin:0;padding:1rem;line-height:1.45}"
            "main{max-width:1120px;margin:0 auto}h1{margin:.2rem 0 1rem;font-size:1.35rem}h2{font-size:1rem;margin:0 0 .8rem}"
            ".tabs{display:flex;gap:.4rem;border-bottom:1px solid #374151;margin-bottom:1rem}.tab{width:auto;margin:0;padding:.7rem 1rem;background:transparent;border:0;border-bottom:3px solid transparent;border-radius:0}.tab.active{border-bottom-color:#22c55e;color:#86efac}"
            ".panel{display:none}.panel.active{display:block}.card{background:#1f2937;border:1px solid #374151;border-radius:.5rem;padding:1rem;margin-bottom:1rem}"
            "label{display:block;margin:.5rem 0 .2rem;font-size:.85rem;color:#cbd5e1}"
            "input,select,button{width:100%;padding:.55rem .7rem;border:1px solid #4b5563;border-radius:.4rem;background:#111827;color:#f9fafb;font:inherit;box-sizing:border-box}"
            "button{background:#2563eb;border-color:#2563eb;color:#fff;font-weight:700;cursor:pointer;margin-top:.6rem}button:hover{background:#1d4ed8}.warn{background:#d97706;border-color:#d97706}"
            ".pin{font-family:monospace;font-size:1.4rem;letter-spacing:.3em;background:#111827;padding:.4rem .8rem;border-radius:.4rem;display:inline-block}.net{display:flex;align-items:center;gap:.6rem;padding:.4rem 0;border-bottom:1px dashed #374151;cursor:pointer}.net:hover{color:#38bdf8}.rssi{font-family:monospace;font-size:.75rem;color:#9ca3af;margin-left:auto}.lock{font-size:.7rem;color:#64748b}"
            ".pill{display:inline-block;padding:2px 8px;border-radius:999px;font-size:.7rem;font-weight:700;text-transform:uppercase}.kv{display:grid;grid-template-columns:150px 1fr;gap:.4rem;margin:.3rem 0;font-size:.85rem}.kv .k{color:#9ca3af}.kv .v{font-family:monospace;color:#f9fafb;word-break:break-all}small{color:#9ca3af}"
            ".row{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.75rem}@media(max-width:760px){.row{grid-template-columns:1fr}}"
            "table{width:100%;border-collapse:collapse;font-size:.9rem}th,td{border-bottom:1px solid #374151;padding:.5rem;text-align:left}th{color:#9ca3af}td.num{text-align:right;font-family:monospace}.sw{width:3rem;padding:.2rem}tr.live{transition:background .2s}"
            "</style></head><body><main><h1>hmyLaser32 Server</h1>"
            "<div class='tabs'><button class='tab active' onclick=\"tab('game',this)\" type='button'>Spiel</button><button class='tab' onclick=\"tab('settings',this)\" type='button'>Einstellungen</button></div>");

  html += F("<section id='game' class='panel active'>");
  if (g_matchPhase == MATCH_IDLE || g_matchPhase == MATCH_DONE) {
    html += F("<div class='card'><form method='POST' action='/api/match/start'><button type='submit'>Warteraum starten</button></form></div>");
  } else {
    html += "<div class='card'><strong>Phase:</strong> ";
    html += phaseName(g_matchPhase);
    html += F("</div>");
  }

  html += F("<div class='card'><h2>Spielparameter</h2><form method='POST' action='/api/settings'><div class='row'>"
            "<div><label>Verteilzeit nach Start (Sek.)</label><input name='lobby' type='number' min='5' max='600' value='");
  html += String((unsigned long)g_settings.lobbySeconds);
  html += F("'></div><div><label>Match-Dauer (Sek.)</label><input name='match' type='number' min='30' max='3600' value='");
  html += String((unsigned long)g_settings.matchSeconds);
  html += F("'></div><div><label>Match Modus</label><select name='mode'>");
  html += String("<option value='free-for-all'") + (strcmp(g_settings.mode, "free-for-all") == 0 ? " selected" : "") + ">Alle gegen Alle</option>";
  html += String("<option value='team'") + (strcmp(g_settings.mode, "team") == 0 ? " selected" : "") + ">Team-Modus</option>";
  html += F("</select></div><div><label>Punkte Zone1 (Brust)</label><input name='zone1' type='number' min='1' max='100' value='");
  html += String((int)g_settings.zonePoints[0]);
  html += F("'></div><div><label>Punkte Zone2 (Schultern)</label><input name='zone2' type='number' min='1' max='100' value='");
  html += String((int)g_settings.zonePoints[1]);
  html += F("'></div><div><label>Punkte Zone3 (Ruecken/Waffe)</label><input name='zone3' type='number' min='1' max='100' value='");
  html += String((int)g_settings.zonePoints[2]);
  html += F("'></div></div><button type='submit'>Einstellungen speichern</button></form></div>");

  html += F("<div class='card'><h2>Bekannte Spieler</h2><form method='POST' action='/api/players'><table><thead><tr><th>Farbe</th><th>ID</th><th>Name</th><th>Team</th></tr></thead><tbody>");
  int visible = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    PlayerSnapshot &p = g_snapshots[i];
    if (p.player[0] == '\0') continue;
    char color[8];
    colorHex(p.color ? p.color : UI_COLORS[visible % 10], color, sizeof(color));
    html += "<tr><td><input class='sw' type='color' name='c"; html += String(i); html += "' value='"; html += color;
    html += "'></td><td><code>"; html += String((unsigned long)p.playerId, HEX); html += "</code></td><td>";
    html += p.player; html += "</td><td><select name='t"; html += String(i); html += "'>";
    for (int t = 0; t < MAX_TEAMS; t++) {
      html += "<option value='"; html += String(t); html += "'";
      if ((int)p.teamIndex == t || (strcmp(g_settings.mode, "free-for-all") == 0 && visible == t)) html += " selected";
      html += ">"; html += String(t + 1); html += "</option>";
    }
    html += "</select></td></tr>";
    visible++;
  }
  if (visible == 0) html += F("<tr><td colspan='4'>Noch keine Spieler gesehen.</td></tr>");
  html += F("</tbody></table><button type='submit'>Aktualisieren</button></form></div>");

  if (g_matchPhase == MATCH_LOBBY) {
    html += F("<div class='card'><form method='POST' action='/api/match/activate'><button type='submit'>Match starten</button></form></div>");
  } else if (g_matchPhase == MATCH_DISTRIBUTING) {
    html += F("<div class='card'><strong>Verteilzeit laeuft.</strong></div>");
  }
  html += F("<div class='card'><h2>Live Statistik</h2><table><thead><tr><th>Rank</th><th>Spieler</th><th>Shots</th><th>RX Hits</th><th>Punkte</th></tr></thead><tbody id='livebody'></tbody></table></div></section>");

  html += F("<section id='settings' class='panel'>");
  appendServerInfo(html);
  html += F("<div class='card'><h2>WLAN Setup</h2><div id='scan'><button onclick='scan()' type='button'>Verfügbare WLANs scannen</button><div id='netlist' style='margin-top:.7rem'></div></div>"
            "<form method='POST' action='/api/wifi' onsubmit='return saveWifi(event)'><label>SSID</label><input name='ssid' id='ssid' required maxlength='32'><label>Passwort</label><input name='psk' id='psk' type='password' maxlength='64'><button type='submit'>WLAN speichern + Neustart</button></form>");
  if (g_wifiConnected) {
    html += F("<form method='POST' action='/api/wifi/forget' style='margin-top:.6rem' onsubmit='return confirm(\"WLAN-Login löschen?\");'><label>PIN</label><input name='pin' type='password' maxlength='8' required><button type='submit' class='warn'>WLAN-Login löschen</button></form>");
  }
  html += F("</div></section>");

  html += F("<script>"
            "function tab(id,btn){document.querySelectorAll('.panel').forEach(p=>p.classList.remove('active'));document.querySelectorAll('.tab').forEach(p=>p.classList.remove('active'));document.getElementById(id).classList.add('active');btn.classList.add('active')}"
            "function scan(){var d=document.getElementById('netlist');d.innerHTML='Scanne...';fetch('/api/scan').then(r=>r.json()).then(list=>{d.innerHTML='';list.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{var e=document.createElement('div');e.className='net';e.innerHTML='<span>'+n.ssid+' <small>(Ch '+(n.channel||'?')+')</small></span>'+(n.secure?'<span class=lock>Lock</span>':'')+'<span class=rssi>'+n.rssi+' dBm</span>';e.onclick=()=>{document.getElementById('ssid').value=n.ssid;document.getElementById('psk').focus()};d.appendChild(e)});});}"
            "function saveWifi(e){e.preventDefault();var f=new FormData(e.target);fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(f)}).then(r=>r.text()).then(t=>{alert('Gespeichert. Gerät startet neu.');});return false}"
            "function esc(s){return String(s||'').replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]))}"
            "function live(){fetch('/api/status').then(r=>r.json()).then(s=>{var b=document.getElementById('livebody');if(!b)return;b.innerHTML='';(s.players||[]).sort((a,b)=>b.points-a.points).forEach((p,i)=>{var tr=document.createElement('tr');tr.className='live';tr.style.background='color-mix(in srgb,'+(p.color||'#374151')+' 18%, transparent)';tr.innerHTML='<td>'+(i+1)+'</td><td>'+esc(p.name)+'</td><td class=num>'+p.shots+'</td><td class=num>'+p.rxHits+'</td><td class=num>'+p.points+'</td>';b.appendChild(tr)});});}setInterval(live,1500);live();"
            "</script></main></body></html>");
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
  if (server.arg("pin") != String(g_serverPin)) {
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
  j += "\",\"mode\":\""; j += g_settings.mode;
  j += "\",\"lobbySeconds\":"; j += String((unsigned long)g_settings.lobbySeconds);
  j += ",\"matchSeconds\":"; j += String((unsigned long)g_settings.matchSeconds);
  j += ",\"hitPoints\":"; j += String((int)g_settings.hitPoints);
  j += ",\"zone1Points\":"; j += String((int)g_settings.zonePoints[0]);
  j += ",\"zone2Points\":"; j += String((int)g_settings.zonePoints[1]);
  j += ",\"zone3Points\":"; j += String((int)g_settings.zonePoints[2]);
  j += ",\"wifiConnected\":"; j += (g_wifiConnected ? "true" : "false");
  j += ",\"portalRegistered\":"; j += (g_portalRegistered ? "true" : "false");
  j += ",\"matchPhase\":\""; j += phaseName(g_matchPhase); j += "\"";
  j += ",\"apSsid\":\""; j += g_apSsid; j += "\"";
  j += ",\"staIp\":\""; j += g_staIp; j += "\"";
  j += ",\"mdns\":\""; j += g_mdnsHost; j += "\",\"players\":[";
  bool first = true;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    PlayerSnapshot &p = g_snapshots[i];
    if (p.player[0] == '\0') continue;
    char color[8];
    colorHex(p.color, color, sizeof(color));
    if (!first) j += ",";
    first = false;
    j += "{\"name\":\""; j += p.player;
    j += "\",\"id\":"; j += String((unsigned long)p.playerId);
    j += ",\"deviceId\":\""; j += String((unsigned long)p.playerId, HEX);
    j += "\",\"team\":"; j += String((int)p.teamIndex + 1);
    j += ",\"color\":\""; j += color;
    j += "\",\"shots\":"; j += String((unsigned)p.shotsFired);
    j += ",\"rxHits\":"; j += String((unsigned)p.rxHits);
    j += ",\"points\":"; j += String((int)p.lastPoints);
    j += "}";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

static void handleSettingsPost() {
  if (server.hasArg("mode")) strlcpy(g_settings.mode, server.arg("mode").c_str(), sizeof(g_settings.mode));
  if (server.hasArg("lobby")) g_settings.lobbySeconds = (uint32_t)server.arg("lobby").toInt();
  if (server.hasArg("match")) g_settings.matchSeconds = (uint32_t)server.arg("match").toInt();
  if (server.hasArg("zone1")) g_settings.zonePoints[0] = (int16_t)server.arg("zone1").toInt();
  if (server.hasArg("zone2")) g_settings.zonePoints[1] = (int16_t)server.arg("zone2").toInt();
  if (server.hasArg("zone3")) g_settings.zonePoints[2] = (int16_t)server.arg("zone3").toInt();
  if (g_settings.zonePoints[0] <= 0) g_settings.zonePoints[0] = DEFAULT_ZONE1_POINTS;
  if (g_settings.zonePoints[1] <= 0) g_settings.zonePoints[1] = DEFAULT_ZONE2_POINTS;
  if (g_settings.zonePoints[2] <= 0) g_settings.zonePoints[2] = DEFAULT_ZONE3_POINTS;
  g_settings.hitPoints = g_settings.zonePoints[0];
  rebuildTeamsFromSnapshots();
  matchSaveSettings();
  espNowBroadcastTeams();
  espNowBroadcastPlayerConfig();
  espNowBroadcastPhase();
  redirectToRoot();
}

static void handlePlayersPost() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    PlayerSnapshot &p = g_snapshots[i];
    if (p.player[0] == '\0') continue;
    if (server.hasArg(String("t") + i)) p.teamIndex = (uint8_t)server.arg(String("t") + i).toInt();
    if (server.hasArg(String("c") + i)) p.color = parseColor(server.arg(String("c") + i), p.color);
  }
  if (strcmp(g_settings.mode, "team") == 0) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (g_snapshots[i].player[0] == '\0') continue;
      uint8_t team = g_snapshots[i].teamIndex;
      uint32_t color = g_snapshots[i].color;
      for (int j = 0; j < MAX_PLAYERS; j++) {
        if (g_snapshots[j].player[0] != '\0' && g_snapshots[j].teamIndex == team) g_snapshots[j].color = color;
      }
    }
  }
  rebuildTeamsFromSnapshots();
  espNowBroadcastTeams();
  espNowBroadcastPlayerConfig();
  redirectToRoot();
}

static void handleMatchStart() {
  if (!matchStart()) {
    server.send(409, "text/plain", "already running");
    return;
  }
  redirectToRoot();
}

static void handleMatchActivate() {
  if (!matchActivate()) {
    server.send(409, "text/plain", "not in waiting room");
    return;
  }
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
  server.on("/api/players", HTTP_POST, handlePlayersPost);
  server.on("/api/match/start", HTTP_POST, handleMatchStart);
  server.on("/api/match/activate", HTTP_POST, handleMatchActivate);
  server.on("/api/status", HTTP_GET, handleStatus);

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
