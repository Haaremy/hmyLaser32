#ifndef LASERTAG_SERVER_GLOBALS_H
#define LASERTAG_SERVER_GLOBALS_H

#include <Arduino.h>
#include "Types.h"

// Identität
extern char g_serverName[IDENTITY_NAME_BYTES];
extern char g_serverPin[IDENTITY_PIN_DIGITS + 1];
extern char g_serverId[40];      // CUID vom Webservice nach erfolgreichem Register

// Verbindung
extern bool g_wifiConnected;
extern bool g_portalRegistered;

// Match-State
extern MatchPhase    g_matchPhase;
extern MatchSettings g_settings;
extern char          g_currentMatchId[40];     // matchId vom Webservice nach match/start
extern unsigned long g_lobbyEndsAtMs;          // millis() ab dem das Match aktiv wird
extern unsigned long g_matchEndsAtMs;          // millis() ab dem das Match endet
extern PlayerSnapshot g_snapshots[MAX_PLAYERS];

#endif
