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

// LED-Hardware
//   1 = WS2812B-Streifen (Standard, v4.2)
//   0 = einzelne 5 mm RGB-LED (Legacy, vor v4.2)
#define USE_WS2812 1

// Anzahl LEDs im Streifen (nur relevant bei USE_WS2812 = 1)
#define LED_STRIP_COUNT 30

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
constexpr uint16_t IR_RECV_PIN = 14;

// Button
//   Default GPIO 32 (NFC-kompatibel — GPIO 19 wäre dort MISO blockiert).
//   Wer KEIN NFC nutzt, kann den Button alternativ auf GPIO 19 setzen.
constexpr uint16_t BUTTON_PIN = 32;

// WS2812B-Datenleitung (gilt wenn USE_WS2812 = 1)
constexpr uint16_t LED_DATA_PIN = 26;

// Legacy 5 mm RGB-LED-Pins (gilt wenn USE_WS2812 = 0)
constexpr uint16_t RGB_RED_PIN   = 25;
constexpr uint16_t RGB_GREEN_PIN = 26;
constexpr uint16_t RGB_BLUE_PIN  = 27;
constexpr bool     RGB_COMMON_ANODE = false;

// NFC (MFRC522 via SPI, nur relevant wenn HAS_NFC = 1)
constexpr uint16_t NFC_SS_PIN  = 5;
constexpr uint16_t NFC_RST_PIN = 33;
// SPI-Pins: MOSI=23, MISO=19, SCK=18 (Hardware-SPI VSPI)

// OLED I²C (Pins 21=SDA, 22=SCL — über Wire.begin in setup)

// ──────────────────────────────────────────────────────────────────────────
//   Konstanten — sollten i.d.R. unverändert bleiben
// ──────────────────────────────────────────────────────────────────────────

constexpr uint8_t WIFI_CHANNEL = 1;
constexpr int MAX_PEERS = 20;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_TEAMS = 4;

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

// Match-Phasen
constexpr uint8_t PHASE_IDLE   = 0;
constexpr uint8_t PHASE_LOBBY  = 1;
constexpr uint8_t PHASE_ACTIVE = 2;
constexpr uint8_t PHASE_DONE   = 3;

// Timing
constexpr unsigned long DEBOUNCE_MS = 50;
constexpr unsigned long SELF_HIT_IGNORE_MS = 200;
constexpr unsigned long HIT_DISABLE_MS = 5000;
constexpr unsigned long PHASE_TIMEOUT_MS = 5000;
constexpr unsigned long TEAMS_TIMEOUT_MS = 8000;
constexpr unsigned long HIT_BLINK_INTERVAL_MS = 120;

// Identity-Aushandlung
constexpr unsigned long IDENTITY_WAIT_MS           = 5000;
constexpr unsigned long DISCOVERY_FAST_INTERVAL_MS = 1000;
constexpr unsigned long DISCOVERY_SLOW_INTERVAL_MS = 5000;
constexpr unsigned long DISCOVERY_FAST_PHASE_MS    = 15000;

// Stand-Alone-Lobby
constexpr unsigned long STANDALONE_LOBBY_SECONDS = 60;
constexpr unsigned long STANDALONE_MATCH_SECONDS = 300;
constexpr unsigned long STANDALONE_TIMEOUT_MS    = 4000;
constexpr unsigned long STANDALONE_BROADCAST_MS  = 1000;

#endif
