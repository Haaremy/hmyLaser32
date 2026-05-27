#ifndef LASERTAG_SERVER_ESPNOW_H
#define LASERTAG_SERVER_ESPNOW_H

#include <Arduino.h>
#include "Types.h"

void espNowBegin();
void espNowLoop();

// Phase-Broadcast (current match phase + remaining seconds).
void espNowBroadcastPhase();

// Teams-Broadcast (current team definitions). Wirkt nur, wenn g_settings.teamCount > 0.
void espNowBroadcastTeams();
void espNowBroadcastPlayerConfig();

int espNowGetRanking(PlayerSnapshot *out);

#endif
