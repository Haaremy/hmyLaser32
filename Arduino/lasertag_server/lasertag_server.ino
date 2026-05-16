// ============================================================================
// hmyLaser32 — ESP-Server
// ----------------------------------------------------------------------------
// Modulares Arduino-Projekt für einen ESP32 NodeMCU-32S, das als Bridge
// zwischen den ESP-NOW-Clients und dem Web-Portal (laser32.haaremy.de) dient.
//
// Setup
//   1. Erstmaliges Anschließen: ESP startet AP `hmyLaser32-XXXX` (offen).
//   2. Mit dem AP verbinden, http://192.168.4.1 öffnen.
//   3. WLAN scannen, eigenes WLAN auswählen, Passwort eingeben → speichern.
//   4. ESP startet neu, verbindet sich, registriert sich automatisch beim
//      Web-Portal (POST /api/bridge/register).
//   5. Auf der Startseite des Portals erscheint dieser Server.
//      Match-Start: lokal über http://<server-ip>/ (WLAN-lokal) ODER über
//      die Portal-UI mit dem PIN.
//
// Identität (Name + PIN) wird beim ersten Boot zufällig generiert und im
// NVS persistent gespeichert. Reset über die lokale Webseite oder per
// vollständigem NVS-Wipe (Flash-Erase beim Re-Flash).
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

MatchPhase    g_matchPhase = MATCH_IDLE;
MatchSettings g_settings   = { "free-for-all", DEFAULT_LOBBY_SECONDS, DEFAULT_MATCH_SECONDS };
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

  // Versuche STA-Verbindung mit gespeicherten Creds
  bool sta = wifiTryStation();
  if (!sta) {
    wifiStartCaptive();
  }

  // ESP-NOW läuft sowohl im AP als auch im STA-Mode auf demselben Kanal
  espNowBegin();

  // Webserver für Konfig + Match-Trigger
  portalBegin();

  // Initial-Registrierung (nur im STA-Mode möglich)
  if (g_wifiConnected) {
    if (!bridgeRegister()) {
      LT_LOG("Initial register failed — will retry");
    }
  }
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastRegisterRetry = 0;

  wifiLoop();
  portalLoop();
  espNowLoop();
  matchLoop();

  // Heartbeat zum Portal alle HEARTBEAT_INTERVAL_MS
  if (g_wifiConnected && g_portalRegistered &&
      millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
    lastHeartbeat = millis();
    bridgeHeartbeat();
  }

  // Bei verlorener Registrierung erneut versuchen
  if (g_wifiConnected && !g_portalRegistered &&
      millis() - lastRegisterRetry > PORTAL_REGISTER_RETRY_MS) {
    lastRegisterRetry = millis();
    bridgeRegister();
  }
}
