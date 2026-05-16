#ifndef LASERTAG_CONFIG_H
#define LASERTAG_CONFIG_H

#include <Arduino.h>

// ============================================================================
//   hmyLaser32 — Client-Konfiguration (v2: phase-aware)
// ----------------------------------------------------------------------------
//   PRO GERÄT individuell setzen:
//     - MY_NAME           — Anzeigename auf dem OLED + im Ranking
//     - MY_IR_COMMAND     — eindeutiger NEC-Command (0x01..0xFE) pro Gerät
// ============================================================================

constexpr uint8_t WIFI_CHANNEL = 1;  // Muss zum Server-Channel passen
constexpr int MAX_PEERS = 20;
constexpr int MAX_PLAYERS = 10;

// --- Pin-Belegung (NodeMCU-32S) -------------------------------------------
constexpr uint16_t IR_SEND_PIN = 4;
constexpr uint16_t IR_RECV_PIN = 14;
constexpr uint16_t BUTTON_PIN = 19;
constexpr uint16_t RGB_RED_PIN = 25;
constexpr uint16_t RGB_GREEN_PIN = 26;
constexpr uint16_t RGB_BLUE_PIN = 27;
constexpr bool RGB_COMMON_ANODE = false;   // 5 mm RGB-LED mit gemeinsamer Kathode

// --- Spieler-Identität (pro Gerät anpassen) -------------------------------
constexpr char MY_NAME[] = "PLAYER_1";
constexpr uint16_t IR_GAME_ADDRESS = 0x00FF;
constexpr uint8_t MY_IR_COMMAND = 0x01;

// --- ESP-NOW Message-Typen ------------------------------------------------
constexpr uint8_t MSG_DISCOVERY = 0;
constexpr uint8_t MSG_TABLE     = 1;
// NEU (v2): Server broadcastet aktuelle Match-Phase + Restzeit
constexpr uint8_t MSG_PHASE     = 2;

// --- Match-Phasen (Server-gesteuert) --------------------------------------
constexpr uint8_t PHASE_IDLE   = 0;
constexpr uint8_t PHASE_LOBBY  = 1;
constexpr uint8_t PHASE_ACTIVE = 2;
constexpr uint8_t PHASE_DONE   = 3;

// --- Timing ---------------------------------------------------------------
constexpr unsigned long DEBOUNCE_MS = 50;
constexpr unsigned long SELF_HIT_IGNORE_MS = 200;
constexpr unsigned long HIT_DISABLE_MS = 5000;

// Wenn länger als dieser Zeitraum keine Phase-Nachricht vom Server eintrifft,
// fällt der Client zurück auf v1-Stand-Alone-Modus — alle Punkte zählen,
// das Spiel funktioniert auch ohne ESP-Server.
constexpr unsigned long PHASE_TIMEOUT_MS = 5000;

#endif
