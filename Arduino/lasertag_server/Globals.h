#ifndef LASERTAG_SERVER_GLOBALS_H
#define LASERTAG_SERVER_GLOBALS_H

#include <Arduino.h>
#include "Types.h"

// Identität
extern char g_serverName[IDENTITY_NAME_BYTES];
extern char g_serverPin[IDENTITY_PIN_DIGITS + 1];
extern char g_serverId[40];      // CUID vom Webservice nach erfolgreichem Register

// Netzwerk-Status
extern bool g_wifiConnected;
extern bool g_portalRegistered;
extern char g_apSsid[40];
extern char g_staIp[20];
extern char g_mdnsHost[48];
extern uint8_t g_staChannel;

// Match-State
extern MatchPhase    g_matchPhase;
extern MatchSettings g_settings;
extern char          g_currentMatchId[40];
extern unsigned long g_lobbyEndsAtMs;
extern unsigned long g_matchEndsAtMs;
extern PlayerSnapshot g_snapshots[MAX_PLAYERS];

#endif
