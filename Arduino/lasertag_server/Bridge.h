#ifndef LASERTAG_SERVER_BRIDGE_H
#define LASERTAG_SERVER_BRIDGE_H

#include <Arduino.h>

bool bridgeRegister();
bool bridgeHeartbeat();
bool bridgeStartMatch();

struct EndPlayer {
  const char *nfc;
  const char *team;
  int hits;
  int deaths;
  int points;
};
bool bridgeEndMatch(const EndPlayer *players, int count);
bool bridgeForwardHit(const char *shooterNfc, const char *targetNfc, int points);

// Pusht die Liste der bekannten Clients (g_snapshots) an den Webservice.
// Wird periodisch in loop() aufgerufen, damit das Web-UI die verbundenen
// Geräte im Team-Editor zur Auswahl anbieten kann.
bool bridgePushKnownPlayers();

// Pulled die ESP-Settings (mode/lobby/match/start-request) vom Webservice.
// Liefert true bei erfolgreichem Update der g_settings; setzt zudem
// `out_startRequested` wenn der Webservice ein Match-Start anstößt.
bool bridgePullSettings(bool &out_startRequested);

#endif
