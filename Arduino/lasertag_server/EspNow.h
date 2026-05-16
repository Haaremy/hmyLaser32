#ifndef LASERTAG_SERVER_ESPNOW_H
#define LASERTAG_SERVER_ESPNOW_H

#include <Arduino.h>
#include "Types.h"

void espNowBegin();
void espNowLoop();

// Aktueller Server-seitiger Snapshot (in Globals abgelegt).
// `out` muss MAX_PLAYERS Slots haben. Liefert Anzahl belegter Einträge.
int espNowGetRanking(PlayerSnapshot *out);

#endif
