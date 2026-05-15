#ifndef LASERTAG_RANKING_H
#define LASERTAG_RANKING_H

#include <Arduino.h>
#include "Types.h"

int findPlayerIndexById(uint32_t playerId);
bool upsertRankingEntry(uint32_t playerId, const char *playerName, int points, uint32_t lastUpdate);
void queueTableBroadcast();
void updateMyScore(int points);
int getMyRank();
int getPointsById(uint32_t playerId);
void syncMyPointsFromRanking();
const char *findPlayerNameById(uint32_t playerId);
bool awardPointsToPlayer(uint32_t playerId, const char *playerName, int deltaPoints);
void printRanking();

#endif
