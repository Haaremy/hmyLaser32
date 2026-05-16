#ifndef LASERTAG_TYPES_H
#define LASERTAG_TYPES_H

#include <Arduino.h>
#include "Config.h"

// Wire-Format-Strukturen — IDENTISCH zum Server. Bei Änderungen Server
// gleichermaßen migrieren!

struct __attribute__((packed)) RankingEntry {
  uint32_t playerId;
  char     player[12];
  int16_t  points;
  uint32_t lastUpdate;
};

struct __attribute__((packed)) Message {
  char        senderName[12];
  uint8_t     msgType;
  uint8_t     playerCount;
  RankingEntry entries[MAX_PLAYERS];
};

// Slot-Belegung MSG_PHASE:
//   entries[0].playerId = MatchPhase (0..3)
//   entries[0].points   = Sekunden bis nächster Phasenwechsel
//   entries[0].player   = Modus-Name
//
// Slot-Belegung MSG_TEAMS (eine Slot pro Team):
//   entries[i].player     = Team-Name
//   entries[i].playerId   = 0x00RRGGBB Farbe
//   entries[i].lastUpdate = Member-Bitmask (Bit n = MY_IR_COMMAND n+1)

struct TeamDef {
  char     name[12];
  uint32_t color;
  uint32_t memberBits;
};

#endif
