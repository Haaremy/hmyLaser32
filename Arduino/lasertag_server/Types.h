#ifndef LASERTAG_SERVER_TYPES_H
#define LASERTAG_SERVER_TYPES_H

#include <Arduino.h>
#include "Config.h"

// Identisches Wire-Format wie der Client (Network.cpp / Types.h dort).
struct __attribute__((packed)) RankingEntry {
  uint32_t playerId;
  char     player[12];
  int16_t  points;
  uint32_t lastUpdate;
};

struct __attribute__((packed)) Message {
  char        senderName[12];
  uint8_t     msgType;        // 0 = DISCOVERY, 1 = TABLE
  uint8_t     playerCount;
  RankingEntry entries[MAX_PLAYERS];
};

constexpr uint8_t MSG_DISCOVERY = 0;
constexpr uint8_t MSG_TABLE     = 1;

// Lokaler Snapshot zum Diff-Detect: jeder Spieler bekommt einen letzten
// bekannten Punktestand zugeordnet; sobald die Punktzahl steigt, wird ein
// Hit-Event an den Cloud-Bridge geschickt.
struct PlayerSnapshot {
  uint32_t playerId;
  char     player[12];
  int16_t  lastPoints;
  uint32_t lastUpdate;
};

enum MatchPhase : uint8_t {
  MATCH_IDLE   = 0,
  MATCH_LOBBY  = 1,   // Lobby-Timer läuft (verteilen + verstecken)
  MATCH_ACTIVE = 2,   // Spiel läuft
  MATCH_DONE   = 3    // beendet, wartet auf Cleanup
};

struct MatchSettings {
  char     mode[24];
  uint32_t lobbySeconds;
  uint32_t matchSeconds;
};

#endif
