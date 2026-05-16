#ifndef LASERTAG_GLOBALS_H
#define LASERTAG_GLOBALS_H

#include <Arduino.h>
#include <esp_now.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <U8g2lib.h>
#include "Config.h"
#include "Types.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IRsend irsend;
extern IRrecv irrecv;
extern decode_results results;

extern bool lastButtonState;
extern bool lastRawButtonReading;
extern unsigned long lastDebounce;
extern unsigned long lastDisplayRefreshAt;

extern int hitCount;
extern int shotsFired;
extern int myPoints;
extern unsigned long lastShotAt;
extern unsigned long playerDisabledUntil;

extern RankingEntry ranking[MAX_PLAYERS];
extern Message outgoing;
extern uint8_t knownPeers[MAX_PEERS][6];
extern int peerCount;
extern bool pendingTableBroadcast;
extern unsigned long lastTableBroadcast;
extern volatile bool sendStatusPending;
extern volatile esp_now_send_status_t lastSendStatus;

// --- Match-Phase (Server-broadcastet, v2) ---------------------------------
extern uint8_t  g_phase;
extern uint32_t g_phaseSecondsLeft;
extern unsigned long g_phaseLastUpdate;
extern char     g_phaseMode[16];

// --- Teams (v3) -----------------------------------------------------------
extern TeamDef  g_teams[MAX_TEAMS];
extern uint8_t  g_teamCount;
extern int8_t   g_myTeamIndex;
extern unsigned long g_teamsLastUpdate;

// --- Hit-Effekt (v4) ------------------------------------------------------
extern uint32_t g_hitBlinkColor;
extern unsigned long g_hitBlinkUntilMs;
extern int g_hitBlinkRemaining;

// --- Stand-Alone-Lobby (v4 + v4.1 Master-Election) ------------------------
extern uint8_t  g_standalonePhase;
extern unsigned long g_standaloneLobbyEnds;
extern unsigned long g_standaloneMatchEnds;
extern unsigned long g_standaloneLastUpdate;   // v4.1: letzte MSG_STANDALONE-Empfang

#endif
