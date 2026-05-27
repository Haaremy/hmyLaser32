#include "Ranking.h"
#include <cstring>
#include "Config.h"
#include "Game.h"
#include "Globals.h"
#include "Identity.h"

int findPlayerIndexById(uint32_t playerId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (ranking[i].player[0] != '\0' && ranking[i].playerId == playerId) {
      return i;
    }
  }
  return -1;
}

int findFreeSlot() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (ranking[i].player[0] == '\0') {
      return i;
    }
  }
  return -1;
}

void sortRanking() {
  for (int i = 0; i < MAX_PLAYERS - 1; i++) {
    for (int j = i + 1; j < MAX_PLAYERS; j++) {
      if (ranking[j].player[0] == '\0') {
        continue;
      }
      if (ranking[i].player[0] == '\0' || ranking[j].points > ranking[i].points) {
        RankingEntry temp = ranking[i];
        ranking[i] = ranking[j];
        ranking[j] = temp;
      }
    }
  }
}

bool shouldReplaceEntry(const RankingEntry &currentEntry, const RankingEntry &incomingEntry) {
  if (currentEntry.player[0] == '\0') {
    return true;
  }

  if (currentEntry.playerId != incomingEntry.playerId) {
    return false;
  }

  // Punkte sind monoton steigend. `lastUpdate` kommt von millis() des
  // jeweiligen ESP und ist geraeteuebergreifend nicht direkt vergleichbar.
  if (incomingEntry.points > currentEntry.points) {
    return true;
  }

  if (incomingEntry.lastUpdate > currentEntry.lastUpdate) {
    return true;
  }

  return false;
}

bool upsertRankingEntry(uint32_t playerId, const char *playerName, int points, uint32_t lastUpdate) {
  int index = findPlayerIndexById(playerId);
  if (index == -1) {
    index = findFreeSlot();
  }

  if (index == -1) {
    Serial.println("[RANKING] Ranking full");
    return false;
  }

  RankingEntry incomingEntry = {};
  incomingEntry.playerId = playerId;
  strncpy(incomingEntry.player, playerName, sizeof(incomingEntry.player) - 1);
  incomingEntry.player[sizeof(incomingEntry.player) - 1] = '\0';
  incomingEntry.points = points;
  incomingEntry.lastUpdate = lastUpdate;

  if (!shouldReplaceEntry(ranking[index], incomingEntry) &&
      ranking[index].playerId == incomingEntry.playerId) {
    return false;
  }

  ranking[index] = incomingEntry;
  sortRanking();
  return true;
}

void queueTableBroadcast() {
  pendingTableBroadcast = true;
}

void updateMyScore(int points) {
  if (upsertRankingEntry(getMyPlayerId(), identityMyName(), points, millis())) {
    queueTableBroadcast();
  }
}

int getMyRank() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (ranking[i].player[0] != '\0' && ranking[i].playerId == getMyPlayerId()) {
      return i + 1;
    }
  }
  return -1;
}

int getPointsById(uint32_t playerId) {
  int index = findPlayerIndexById(playerId);
  if (index == -1) {
    return 0;
  }
  return ranking[index].points;
}

void syncMyPointsFromRanking() {
  int index = findPlayerIndexById(getMyPlayerId());
  if (index != -1) {
    myPoints = ranking[index].points;
  }
}

const char *findPlayerNameById(uint32_t playerId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (ranking[i].player[0] != '\0' && ranking[i].playerId == playerId) {
      return ranking[i].player;
    }
  }
  return nullptr;
}

bool awardPointsToPlayer(uint32_t playerId, const char *playerName, int deltaPoints) {
  int currentPoints = getPointsById(playerId);
  bool changed = upsertRankingEntry(playerId, playerName, currentPoints + deltaPoints, millis());
  if (changed) {
    syncMyPointsFromRanking();
  }
  return changed;
}

void printRanking() {
  Serial.println("[RANKING]");
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (ranking[i].player[0] == '\0') {
      continue;
    }

    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(ranking[i].player);
    Serial.print(" [0x");
    Serial.print(ranking[i].playerId, HEX);
    Serial.print("] - ");
    Serial.print(ranking[i].points);
    Serial.print(" pts");
    if (ranking[i].playerId == getMyPlayerId()) {
      Serial.print(" <- me");
    }
    Serial.println();
  }
}
