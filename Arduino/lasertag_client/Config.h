#ifndef LASERTAG_CONFIG_H
#define LASERTAG_CONFIG_H

#include <Arduino.h>

// ============================================================================
//   hmyLaser32 — Client-Konfiguration
//   Alles was du normalerweise pro Build anpasst, steht in der QUICK-CONFIG
//   weiter unten. Die Spielerdaten werden ab v4 automatisch ausgehandelt —
//   keine Hardcoding von MY_NAME / MY_IR_COMMAND mehr.
// ============================================================================

// ──────────────────────────────────────────────────────────────────────────
//   QUICK-CONFIG — die einzigen Schalter, die du normalerweise umsetzen musst
// ──────────────────────────────────────────────────────────────────────────

// LED-Hardware: WS2812B-Streifen
#define LED_STRIP_COUNT 80

// NFC-Reader (MFRC522)
//   1 = vorhanden, NFC-Account-Binding aktiv
//   0 = nicht vorhanden (Default, V1-Bauplan ohne NFC)
#define HAS_NFC 0

// Helligkeit des WS2812B-Streifens (0–255). Reduziert Stromverbrauch.
#define LED_BRIGHTNESS 80

// ──────────────────────────────────────────────────────────────────────────
//   Hardware-Pinout — selten zu ändern, gilt für NodeMCU-32S
// ──────────────────────────────────────────────────────────────────────────

// IR
constexpr uint16_t IR_SEND_PIN = 4;
constexpr uint16_t IR_RECV_PIN = 13;
constexpr uint16_t IR_RECV_PIN_SECONDARY = 26;
constexpr uint16_t IR_RECV_PIN_TERTIARY = 25;

// Button
//   Default GPIO 32 (NFC-kompatibel — GPIO 19 wäre dort MISO blockiert).
//   Wer KEIN NFC nutzt, kann den Button alternativ auf GPIO 19 setzen.
constexpr uint16_t BUTTON_PIN = 32;

// WS2812B-Datenleitung
// GPIO 12 ist ein ESP32 Boot-Strapping-Pin und kann Upload/Boot stoeren.
constexpr uint16_t LED_DATA_PIN = 27;

// NFC (MFRC522 via SPI, nur relevant wenn HAS_NFC = 1)
constexpr uint16_t NFC_SS_PIN  = 5;
constexpr uint16_t NFC_RST_PIN = 33;
// SPI-Pins: MOSI=23, MISO=19, SCK=18 (Hardware-SPI VSPI)

// OLED I²C (Pins 21=SDA, 22=SCL — über Wire.begin in setup)

// ──────────────────────────────────────────────────────────────────────────
//   Konstanten — sollten i.d.R. unverändert bleiben
// ──────────────────────────────────────────────────────────────────────────

constexpr uint8_t WIFI_CHANNEL = 1;
constexpr char SERVER_AP_SSID_PREFIX[] = "hmyLaser32-";
constexpr int MAX_PEERS = 20;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_TEAMS = 10;

// Make the QUICK-CONFIG flags available as constexpr too
constexpr int      LED_COUNT      = LED_STRIP_COUNT;

// IR
constexpr uint16_t IR_GAME_ADDRESS = 0x00FF;

// ESP-NOW Message-Typen
constexpr uint8_t MSG_DISCOVERY  = 0;
constexpr uint8_t MSG_TABLE      = 1;
constexpr uint8_t MSG_PHASE      = 2;
constexpr uint8_t MSG_TEAMS      = 3;
constexpr uint8_t MSG_NFC        = 4;
constexpr uint8_t MSG_STANDALONE = 5;   // v4.1 — Lobby-Master Konsens
constexpr uint8_t MSG_PLAYER_STATE  = 6;
constexpr uint8_t MSG_PLAYER_CONFIG = 7;
constexpr uint8_t MSG_HIT_EVENT     = 8;

// Match-Phasen
constexpr uint8_t PHASE_IDLE   = 0;
constexpr uint8_t PHASE_LOBBY  = 1;
constexpr uint8_t PHASE_DISTRIBUTING = 2;
constexpr uint8_t PHASE_ACTIVE = 3;
constexpr uint8_t PHASE_DONE   = 4;

// Timing
constexpr unsigned long DEBOUNCE_MS = 50;
constexpr unsigned long SELF_HIT_IGNORE_MS = 200;
constexpr unsigned long HIT_DISABLE_MS = 5000;
constexpr unsigned long PHASE_TIMEOUT_MS = 5000;
constexpr unsigned long TEAMS_TIMEOUT_MS = 8000;
constexpr unsigned long HIT_BLINK_INTERVAL_MS = 120;
constexpr int DEFAULT_ZONE1_POINTS = 15;  // Brust
constexpr int DEFAULT_ZONE2_POINTS = 10;  // Schultern
constexpr int DEFAULT_ZONE3_POINTS = 5;   // Ruecken / Waffe
constexpr int DEFAULT_HIT_POINTS = DEFAULT_ZONE1_POINTS;

// Identity-Aushandlung
constexpr unsigned long IDENTITY_WAIT_MS           = 5000;
constexpr unsigned long DISCOVERY_FAST_INTERVAL_MS = 1000;
constexpr unsigned long DISCOVERY_SLOW_INTERVAL_MS = 5000;
constexpr unsigned long DISCOVERY_FAST_PHASE_MS    = 15000;

// Stand-Alone-Lobby
constexpr unsigned long STANDALONE_LOBBY_SECONDS = 20;
constexpr unsigned long STANDALONE_MATCH_SECONDS = 300;
constexpr unsigned long STANDALONE_TIMEOUT_MS    = 4000;
constexpr unsigned long STANDALONE_BROADCAST_MS  = 1000;

#endif
