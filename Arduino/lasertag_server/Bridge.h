#ifndef LASERTAG_SERVER_BRIDGE_H
#define LASERTAG_SERVER_BRIDGE_H

#include <Arduino.h>

// HTTP-Client zum hmyLaser32-Webservice. Alle Calls erfordern aktive
// WLAN-Verbindung (g_wifiConnected). Bei TLS wird das CA-Root mitgeführt
// (root.pem-Konstante via Config oder System).

// Self-Registration: POST /api/bridge/register
// Setzt g_portalRegistered + g_serverId bei Erfolg.
bool bridgeRegister();

// Heartbeat: GET /api/bridge/register
bool bridgeHeartbeat();

// Match-Start: POST /api/bridge/match/start
// Schreibt matchId in g_currentMatchId. Modus/Dauer aus g_settings.
bool bridgeStartMatch();

// Match-End: POST /api/bridge/match/end mit finalem Ranking.
// `entries` ist eine Snapshot-Kopie zum Zeitpunkt des Endes.
struct EndPlayer {
  const char *nfc;
  const char *team;
  int hits;
  int deaths;
  int points;
};
bool bridgeEndMatch(const EndPlayer *players, int count);

// Hit-Event-Forward: POST /api/bridge/hit
// Wird vom Match-Diff-Detector gerufen, sobald ein Score-Diff erkannt wurde.
bool bridgeForwardHit(const char *shooterNfc, const char *targetNfc, int points);

#endif
