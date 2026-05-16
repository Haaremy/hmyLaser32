#ifndef LASERTAG_SERVER_ESPNOW_H
#define LASERTAG_SERVER_ESPNOW_H

#include <Arduino.h>
#include "Types.h"

void espNowBegin();
void espNowLoop();

// Sendet MSG_PHASE an alle Clients (broadcast).
void espNowBroadcastPhase();

// Aktueller Server-seitiger Snapshot.
int espNowGetRanking(PlayerSnapshot *out);

#endif
