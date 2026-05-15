#ifndef LASERTAG_TYPES_H
#define LASERTAG_TYPES_H

#include <Arduino.h>
#include "Config.h"

struct __attribute__((packed)) RankingEntry {
  uint32_t playerId;
  char player[12];
  int16_t points;
  uint32_t lastUpdate;
};

struct __attribute__((packed)) Message {
  char senderName[12];
  uint8_t msgType;
  uint8_t playerCount;
  RankingEntry entries[MAX_PLAYERS];
};

#endif
