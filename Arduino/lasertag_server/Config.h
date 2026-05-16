#ifndef LASERTAG_SERVER_CONFIG_H
#define LASERTAG_SERVER_CONFIG_H

#include <Arduino.h>

// ============================================================================
// hmyLaser32 — ESP-Server Konfiguration
// ----------------------------------------------------------------------------
// Der Server ist ein dedizierter ESP32 NodeMCU-32S ohne IR-Hardware.
// Aufgaben:
//   1. ESP-NOW-Listener (Channel WIFI_CHANNEL, identisch zu allen Clients)
//   2. ESP-NOW-Sender für Match-Phase-Broadcasts an die Clients
//   3. WLAN-Bridge zum Web-Portal (laser32.haaremy.de)
//   4. Permanenter AP + Captive Portal für lokales Setup / Match-Steuerung
//   5. mDNS-Hostname `<name>.local` für komfortablen LAN-Zugriff
//   6. Match-Orchestrierung mit Lobby-Timer (Verteilen + Verstecken)
//
// Hardware (Minimal):
//   - ESP32 NodeMCU-32S
//   - Powerbank
//   (Optional: OLED + Button + RGB-LED — s. Wiki-Erweiterung)
// ============================================================================

// --- ESP-NOW ---------------------------------------------------------------
// AP-Channel; STA-Verbindungen sollten möglichst auf demselben Channel sein,
// sonst kollidieren AP und STA (ESP32 kann im AP+STA-Mode nur einen Channel).
constexpr uint8_t WIFI_CHANNEL = 1;
constexpr int MAX_PLAYERS = 10;

// --- Web-Portal (Cloud-Bridge) ---------------------------------------------
constexpr char PORTAL_BASE_URL[] = "https://laser32.haaremy.de";

constexpr char EP_REGISTER[]      = "/api/bridge/register";
constexpr char EP_MATCH_START[]   = "/api/bridge/match/start";
constexpr char EP_MATCH_END[]     = "/api/bridge/match/end";
constexpr char EP_HIT[]           = "/api/bridge/hit";
constexpr char EP_ESP_SETTINGS[]  = "/api/esp/by-pin/settings";   // GET

// --- Identity (Name + PIN) -------------------------------------------------
constexpr int IDENTITY_PIN_DIGITS = 4;
constexpr int IDENTITY_NAME_BYTES = 24;

// --- AP / Captive Portal ---------------------------------------------------
// Der AP bleibt PERMANENT aktiv, auch wenn STA verbunden ist.
// Dadurch kann der User jederzeit per offenen Hotspot aufs Setup-Web zugreifen.
constexpr char AP_SSID_PREFIX[]   = "hmyLaser32-";   // + 4 zufällige Hexstellen
constexpr char AP_PASSWORD[]      = "";              // leer = offen
constexpr uint16_t AP_IP[]        = { 192, 168, 4, 1 };
constexpr unsigned long STA_CONNECT_TIMEOUT_MS = 20000;

// --- mDNS ------------------------------------------------------------------
// Hostname für LAN-Discovery. Wird zur Laufzeit um den Servernamen ergänzt:
// `hmylaser32-<name>.local`. Erreichbar im selben WLAN.
constexpr char MDNS_PREFIX[]      = "hmylaser32";

// --- Match-Lobby Defaults --------------------------------------------------
constexpr unsigned long DEFAULT_LOBBY_SECONDS    = 60;
constexpr unsigned long DEFAULT_MATCH_SECONDS    = 300;
constexpr char          DEFAULT_MATCH_MODE[]     = "free-for-all";

// --- Periodische Tasks -----------------------------------------------------
constexpr unsigned long HEARTBEAT_INTERVAL_MS    = 30000;
constexpr unsigned long SETTINGS_PULL_INTERVAL_MS = 30000;
constexpr unsigned long PORTAL_REGISTER_RETRY_MS = 15000;
constexpr unsigned long PHASE_BROADCAST_MS       = 1500;   // Re-broadcast aktuelle Phase

// --- Logging ---------------------------------------------------------------
#define LT_LOG(fmt, ...) Serial.printf("[%s] " fmt "\n", __func__, ##__VA_ARGS__)

#endif
