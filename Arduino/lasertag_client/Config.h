#ifndef LASERTAG_CONFIG_H
#define LASERTAG_CONFIG_H

#include <Arduino.h>

// ============================================================================
//   hmyLaser32 — Client-Konfiguration (v4: auto-identity, hit-effect, NFC)
//   Pro Gerät müssen keine Spielerdaten mehr hardcoded werden — Name und
//   Farbe werden zur Lobby-Zeit ausgehandelt (s. Identity.cpp).
// ============================================================================

constexpr uint8_t WIFI_CHANNEL = 1;
constexpr int MAX_PEERS = 20;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_TEAMS = 4;

// --- Pin-Belegung (NodeMCU-32S) -------------------------------------------
// HINWEIS v4: Button auf GPIO 32 verlegt, damit MFRC522-NFC-Reader (optional)
// die SPI-Pins 18/19/23 freilässt. Aktualisiere die HW entsprechend, oder
// behalte GPIO 19 ohne NFC-Erweiterung.
constexpr uint16_t IR_SEND_PIN = 4;
constexpr uint16_t IR_RECV_PIN = 14;
constexpr uint16_t BUTTON_PIN  = 32;       // war 19 (vor v4)
constexpr uint16_t RGB_RED_PIN = 25;
constexpr uint16_t RGB_GREEN_PIN = 26;
constexpr uint16_t RGB_BLUE_PIN = 27;
constexpr bool     RGB_COMMON_ANODE = false;

// --- NFC (optional) -------------------------------------------------------
// Wenn ein MFRC522 angeschlossen ist: HAS_NFC auf 1 setzen. Sonst bleibt der
// Code skip-fähig (NFC-Funktionen sind no-ops).
#define HAS_NFC 0
constexpr uint16_t NFC_SS_PIN  = 5;
constexpr uint16_t NFC_RST_PIN = 33;
// SPI MOSI=23, MISO=19, SCK=18 (Hardware-SPI VSPI)

// --- IR-Identifikation ----------------------------------------------------
// IR_GAME_ADDRESS ist konstant; das Command-Byte ergibt sich aus der
// ausgehandelten Identität (Identity.cpp::identityMyCommand()).
constexpr uint16_t IR_GAME_ADDRESS = 0x00FF;

// --- ESP-NOW Message-Typen ------------------------------------------------
constexpr uint8_t MSG_DISCOVERY = 0;
constexpr uint8_t MSG_TABLE     = 1;
constexpr uint8_t MSG_PHASE     = 2;
constexpr uint8_t MSG_TEAMS     = 3;
constexpr uint8_t MSG_NFC       = 4;       // v4: NFC-Account-Bind-Push

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
constexpr unsigned long HIT_BLINK_INTERVAL_MS = 120;   // 3x blink → ~720 ms total

// --- Stand-Alone-Lobby ----------------------------------------------------
// Ohne Server starten die Clients ihre eigene Lobby, sobald ≥2 Peers
// gefunden wurden. Match-Dauer ist hardcoded; Web-Settings nur via Server.
constexpr unsigned long STANDALONE_LOBBY_SECONDS = 60;
constexpr unsigned long STANDALONE_MATCH_SECONDS = 300;

#endif
