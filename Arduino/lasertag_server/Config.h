#ifndef LASERTAG_SERVER_CONFIG_H
#define LASERTAG_SERVER_CONFIG_H

#include <Arduino.h>

// ============================================================================
// hmyLaser32 — ESP-Server Konfiguration (v3: team mode)
// ============================================================================

// --- ESP-NOW ---------------------------------------------------------------
constexpr uint8_t WIFI_CHANNEL = 1;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_TEAMS   = 10;

// --- Web-Portal (Cloud-Bridge) ---------------------------------------------
constexpr char PORTAL_BASE_URL[] = "https://laser32.haaremy.de";

constexpr char EP_REGISTER[]      = "/api/bridge/register";
constexpr char EP_MATCH_START[]   = "/api/bridge/match/start";
constexpr char EP_MATCH_END[]     = "/api/bridge/match/end";
constexpr char EP_HIT[]           = "/api/bridge/hit";
constexpr char EP_ESP_SETTINGS[]  = "/api/esp/by-pin/settings";
constexpr char EP_PLAYERS[]       = "/api/bridge/players";    // POST known clients

// --- Identity --------------------------------------------------------------
constexpr int IDENTITY_PIN_DIGITS = 4;
constexpr int IDENTITY_NAME_BYTES = 24;

// --- AP / Captive Portal ---------------------------------------------------
constexpr char AP_SSID_PREFIX[]   = "hmyLaser32-";
constexpr char AP_PASSWORD[]      = "";
constexpr uint16_t AP_IP[]        = { 192, 168, 4, 1 };
constexpr unsigned long STA_CONNECT_TIMEOUT_MS = 20000;

// --- mDNS ------------------------------------------------------------------
constexpr char MDNS_PREFIX[]      = "hmylaser32";

// --- Match-Defaults --------------------------------------------------------
constexpr unsigned long DEFAULT_LOBBY_SECONDS    = 60;    // Starttimer
constexpr unsigned long DEFAULT_MATCH_SECONDS    = 300;   // Runden-Dauer
constexpr int           DEFAULT_HIT_POINTS       = 10;
constexpr char          DEFAULT_MATCH_MODE[]     = "free-for-all";

// --- Periodische Tasks -----------------------------------------------------
constexpr unsigned long HEARTBEAT_INTERVAL_MS    = 30000;
constexpr unsigned long SETTINGS_PULL_INTERVAL_MS = 30000;
constexpr unsigned long PORTAL_REGISTER_RETRY_MS = 15000;
constexpr unsigned long PHASE_BROADCAST_MS       = 1500;
constexpr unsigned long TEAMS_BROADCAST_MS       = 3000;  // Teams alle 3 s re-broadcast
constexpr unsigned long PLAYERS_PUSH_INTERVAL_MS = 30000; // Snapshots an Webservice senden

// --- Logging ---------------------------------------------------------------
#define LT_LOG(fmt, ...) Serial.printf("[%s] " fmt "\n", __func__, ##__VA_ARGS__)

#endif
