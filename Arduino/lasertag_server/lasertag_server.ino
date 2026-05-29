// ============================================================================
// hmyLaser32 — ESP-Server (v3: + Team-Modus, Known-Clients-Push)
// ----------------------------------------------------------------------------
// Modulares Arduino-Projekt für einen ESP32 NodeMCU-32S, das als Bridge
// zwischen den ESP-NOW-Clients und dem Web-Portal (laser32.haaremy.de) dient.
//
// Setup
//   1. Erstmaliges Anschließen: ESP startet AP `hmyLaser32-XXXX` (offen).
//      Der AP BLEIBT auch nach STA-Connect permanent erreichbar.
//   2. Mit dem AP verbinden, http://192.168.4.1 öffnen.
//   3. WLAN scannen, eigenes WLAN auswählen, Passwort eingeben → speichern.
//   4. ESP startet neu, verbindet sich, registriert sich beim Web-Portal.
//   5. Lokaler Zugriff weiterhin via:
//        - http://192.168.4.1 (offener AP, immer erreichbar)
//        - http://<name>.local (mDNS im STA-WLAN)
//        - http://<sta-ip> (Status auf der Web-UI ablesbar)
//
// Identität (Name + PIN) wird beim ersten Boot zufällig generiert und im
// NVS persistent gespeichert.
// ============================================================================

#include <Arduino.h>
#include "Bridge.h"
#include "Config.h"
#include "EspNow.h"
#include "Globals.h"
#include "Identity.h"
#include "Match.h"
#include "Portal.h"
#include "Storage.h"
#include "Types.h"
#include "WiFiSetup.h"

// --- Globale Variablen (Definitionen) --------------------------------------
char g_serverName[IDENTITY_NAME_BYTES] = {0};
char g_serverPin[IDENTITY_PIN_DIGITS + 1] = {0};
char g_serverId[40] = {0};

bool g_wifiConnected   = false;
bool g_portalRegistered = false;
char g_apSsid[40]      = {0};
char g_staIp[20]       = {0};
char g_mdnsHost[48]    = {0};
uint8_t g_staChannel   = WIFI_CHANNEL;

MatchPhase    g_matchPhase = MATCH_IDLE;
MatchSettings g_settings = {
  .mode = "free-for-all",
  .lobbySeconds = DEFAULT_LOBBY_SECONDS,
  .matchSeconds = DEFAULT_MATCH_SECONDS,
  .hitPoints = DEFAULT_HIT_POINTS,
  .zonePoints = { DEFAULT_ZONE1_POINTS, DEFAULT_ZONE2_POINTS, DEFAULT_ZONE3_POINTS },
  .teamCount = 0,
  .teams = {}
};
char          g_currentMatchId[40] = {0};
unsigned long g_lobbyEndsAtMs = 0;
unsigned long g_matchEndsAtMs = 0;
PlayerSnapshot g_snapshots[MAX_PLAYERS] = {};

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("hmyLaser32 ESP-Server booting...");

  storageBegin();
  identityLoadOrCreate();
  matchLoadSettings();

  // AP+STA Dualmode. AP bleibt permanent erreichbar, STA verbindet
  // wenn Credentials vorhanden. ESP-NOW folgt dem aktuellen Channel.
  wifiBegin();
  espNowBegin();
  portalBegin();

  if (g_wifiConnected) {
    if (!bridgeRegister()) {
      LT_LOG("Initial register failed — will retry");
    }
  }
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastRegisterRetry = 0;
  static unsigned long lastSettingsPull = 0;
  static unsigned long lastPlayersPush = 0;

  wifiLoop();
  portalLoop();
  espNowLoop();
  matchLoop();

  // Bekannte Clients an den Webservice melden (für den Team-Editor)
  if (g_wifiConnected && g_portalRegistered &&
      millis() - lastPlayersPush > PLAYERS_PUSH_INTERVAL_MS) {
    lastPlayersPush = millis();
    bridgePushKnownPlayers();
  }

  // Heartbeat zum Portal
  if (g_wifiConnected && g_portalRegistered &&
      millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
    lastHeartbeat = millis();
    bridgeHeartbeat();
  }

  // Re-Register bei verlorener Registrierung
  if (g_wifiConnected && !g_portalRegistered &&
      millis() - lastRegisterRetry > PORTAL_REGISTER_RETRY_MS) {
    lastRegisterRetry = millis();
    bridgeRegister();
  }

  // Settings-Pull vom Webservice — erkennt auch Match-Start-Anforderungen
  if (g_wifiConnected && g_portalRegistered &&
      millis() - lastSettingsPull > SETTINGS_PULL_INTERVAL_MS) {
    lastSettingsPull = millis();
    bool startReq = false;
    if (bridgePullSettings(startReq)) {
      if (startReq && (g_matchPhase == MATCH_IDLE || g_matchPhase == MATCH_DONE)) {
        LT_LOG("Match-Start angefordert vom Webservice");
        matchStart();
      }
    }
  }
}
