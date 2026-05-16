#ifndef LASERTAG_CONFIG_H
#define LASERTAG_CONFIG_H

#include <Arduino.h>

// ============================================================================
//   hmyLaser32 — Client-Konfiguration (v4.1)
//   - Auto-Identity via MAC-Aushandlung (keine MY_NAME-Hardcoding mehr)
//   - Master-Election für Standalone-Lobby (niedrigste MAC → Master,
//     broadcastet MSG_STANDALONE, andere syncen ihre Timer)
// ============================================================================

constexpr uint8_t WIFI_CHANNEL = 1;
constexpr int MAX_PEERS = 20;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_TEAMS = 4;

// --- Pin-Belegung (NodeMCU-32S) -------------------------------------------
// Default-Pinout — identisch zu v1 (Paper).
// Wer NFC nachrüsten will: MFRC522 nutzt MISO=GPIO 19. Dann den Button
// auf GPIO 32 (oder 33) verlegen und unten BUTTON_PIN entsprechend setzen.
constexpr uint16_t IR_SEND_PIN = 4;
constexpr uint16_t IR_RECV_PIN = 14;
constexpr uint16_t BUTTON_PIN  = 19;
constexpr uint16_t RGB_RED_PIN = 25;
constexpr uint16_t RGB_GREEN_PIN = 26;
constexpr uint16_t RGB_BLUE_PIN = 27;
constexpr bool     RGB_COMMON_ANODE = false;

// --- NFC (optional, Hardware nicht im V1-Bauplan) -------------------------
#define HAS_NFC 0
constexpr uint16_t NFC_SS_PIN  = 5;
constexpr uint16_t NFC_RST_PIN = 33;
// Wer NFC nutzen will: HAS_NFC=1, BUTTON_PIN auf 32 verlegen
// SPI-Pins: MOSI=23, MISO=19, SCK=18 (Hardware-SPI VSPI)

// --- IR-Identifikation ----------------------------------------------------
constexpr uint16_t IR_GAME_ADDRESS = 0x00FF;

// --- ESP-NOW Message-Typen ------------------------------------------------
constexpr uint8_t MSG_DISCOVERY  = 0;
constexpr uint8_t MSG_TABLE      = 1;
constexpr uint8_t MSG_PHASE      = 2;
constexpr uint8_t MSG_TEAMS      = 3;
constexpr uint8_t MSG_NFC        = 4;
constexpr uint8_t MSG_STANDALONE = 5;   // v4.1 — Lobby-Master Konsens

// --- Match-Phasen ---------------------------------------------------------
constexpr uint8_t PHASE_IDLE   = 0;
constexpr uint8_t PHASE_LOBBY  = 1;
constexpr uint8_t PHASE_ACTIVE = 2;
constexpr uint8_t PHASE_DONE   = 3;

// --- Timing ---------------------------------------------------------------
constexpr unsigned long DEBOUNCE_MS = 50;
constexpr unsigned long SELF_HIT_IGNORE_MS = 200;
constexpr unsigned long HIT_DISABLE_MS = 5000;
constexpr unsigned long PHASE_TIMEOUT_MS = 5000;
constexpr unsigned long TEAMS_TIMEOUT_MS = 8000;
constexpr unsigned long HIT_BLINK_INTERVAL_MS = 120;

// Identity-Aushandlung: warte 5s nach letztem neuen Peer (mehr Toleranz
// für späte Discoveries), aber sende Discovery alle 1s in den ersten 15s
// (für schnelles gegenseitiges Finden), danach alle 5s.
constexpr unsigned long IDENTITY_WAIT_MS         = 5000;
constexpr unsigned long DISCOVERY_FAST_INTERVAL_MS = 1000;
constexpr unsigned long DISCOVERY_SLOW_INTERVAL_MS = 5000;
constexpr unsigned long DISCOVERY_FAST_PHASE_MS    = 15000;

// --- Stand-Alone-Lobby ----------------------------------------------------
constexpr unsigned long STANDALONE_LOBBY_SECONDS = 60;
constexpr unsigned long STANDALONE_MATCH_SECONDS = 300;
constexpr unsigned long STANDALONE_TIMEOUT_MS    = 4000;   // master ohne MSG_STANDALONE
constexpr unsigned long STANDALONE_BROADCAST_MS  = 1000;   // master sendet 1×/s

#endif
