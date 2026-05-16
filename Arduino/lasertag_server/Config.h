#ifndef LASERTAG_SERVER_CONFIG_H
#define LASERTAG_SERVER_CONFIG_H

#include <Arduino.h>

// ============================================================================
// hmyLaser32 — ESP-Server Konfiguration
// ----------------------------------------------------------------------------
// Der Server ist ein dedizierter ESP32 NodeMCU-32S ohne IR-Hardware.
// Aufgaben:
//   1. ESP-NOW-Lauscher (Channel 1, gleicher wie Clients)
//   2. WLAN-Bridge zum Web-Portal (laser32.haaremy.de)
//   3. Lokaler Webserver / Captive Portal für WLAN-Setup + Match-Settings
//   4. Match-Orchestrierung mit Lobby-Timer (z.B. 60s "verteilen + verstecken")
//
// Hardware (Minimal):
//   - ESP32 NodeMCU-32S
//   - Powerbank
//   (Optional: OLED + Button + RGB-LED -- s. Wiki-Erweiterung)
// ============================================================================

// --- ESP-NOW ---------------------------------------------------------------
constexpr uint8_t WIFI_CHANNEL = 1;       // muss zu Client-Config passen
constexpr int MAX_PLAYERS = 10;           // Slot-Größe Ranking (= Client.Config)

// --- Web-Portal (Cloud-Bridge) ---------------------------------------------
// Pfad ohne trailing slash. Default: das Public-Portal.
constexpr char PORTAL_BASE_URL[] = "https://laser32.haaremy.de";

// HTTP-Endpoints (relativ zu PORTAL_BASE_URL).
constexpr char EP_REGISTER[]      = "/api/bridge/register";
constexpr char EP_MATCH_START[]   = "/api/bridge/match/start";
constexpr char EP_MATCH_END[]     = "/api/bridge/match/end";
constexpr char EP_HIT[]           = "/api/bridge/hit";

// --- Identity (Name + PIN) -------------------------------------------------
// Beide werden beim ERSTEN Boot zufällig erzeugt und in NVS persistent
// gespeichert. Long-Press auf RESET-Pin oder POST /api/identity/reset
// generiert sie neu.
constexpr int IDENTITY_PIN_DIGITS = 4;     // 4-stelliger PIN, 1000-9999
constexpr int IDENTITY_NAME_BYTES = 24;    // max. Servername-Länge

// --- Captive Portal AP (Fallback-Modus) ------------------------------------
// Aktiv, wenn kein STA-Connect nach STA_CONNECT_TIMEOUT_MS möglich war.
constexpr char AP_SSID_PREFIX[]   = "hmyLaser32-";   // + 4 zufällige Hexstellen
constexpr char AP_PASSWORD[]      = "";              // leer = offen
constexpr uint16_t AP_IP[]        = { 192, 168, 4, 1 };
constexpr unsigned long STA_CONNECT_TIMEOUT_MS = 20000;

// --- Match-Lobby Defaults --------------------------------------------------
constexpr unsigned long DEFAULT_LOBBY_SECONDS    = 60;    // Verteil-Phase
constexpr unsigned long DEFAULT_MATCH_SECONDS    = 300;   // 5 min Spielzeit
constexpr char          DEFAULT_MATCH_MODE[]     = "free-for-all";

// --- Periodische Tasks -----------------------------------------------------
constexpr unsigned long HEARTBEAT_INTERVAL_MS   = 30000;  // /api/bridge/register GET
constexpr unsigned long RANKING_DIFF_CHECK_MS   = 500;    // Hit-Detect-Rhythmus
constexpr unsigned long PORTAL_REGISTER_RETRY_MS = 15000; // bei Register-Fail

// --- Logging ---------------------------------------------------------------
#define LT_LOG(fmt, ...) Serial.printf("[%s] " fmt "\n", __func__, ##__VA_ARGS__)

#endif
