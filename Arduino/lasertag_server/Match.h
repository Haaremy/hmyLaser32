#ifndef LASERTAG_SERVER_MATCH_H
#define LASERTAG_SERVER_MATCH_H

#include <Arduino.h>
#include "Types.h"

// Lädt Match-Defaults aus NVS in g_settings.
void matchLoadSettings();

// Persistiert g_settings nach NVS.
void matchSaveSettings();

// Startet ein Match in der Lobby-Phase (lobbyEndsAtMs gesetzt).
// Sobald die Lobby endet, wird /api/bridge/match/start ausgelöst und in
// die ACTIVE-Phase übergegangen.
bool matchStart();

// Hartes Ende des Matches: schickt match/end mit aktuellem Ranking.
bool matchEnd();

// Tick: prüft Lobby-Ablauf, Match-Timer, sendet Hit-Diffs (kommt aus EspNow).
void matchLoop();

#endif
