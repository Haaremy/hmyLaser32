#ifndef LASERTAG_SERVER_TYPES_H
#define LASERTAG_SERVER_TYPES_H

#include <Arduino.h>
#include "Config.h"

// Identisches Wire-Format wie der Client (Types.h dort).
// Beachte: bestehende Clients (v1) kennen nur MSG_DISCOVERY und MSG_TABLE.
// Neue Clients (v2) verstehen MSG_PHASE.
struct __attribute__((packed)) RankingEntry {
  uint32_t playerId;
  char     player[12];
  int16_t  points;
  uint32_t lastUpdate;
};

struct __attribute__((packed)) Message {
  char        senderName[12];
  uint8_t     msgType;          // 0 = DISCOVERY, 1 = TABLE, 2 = PHASE
  uint8_t     playerCount;
  RankingEntry entries[MAX_PLAYERS];
};

constexpr uint8_t MSG_DISCOVERY = 0;
constexpr uint8_t MSG_TABLE     = 1;
constexpr uint8_t MSG_PHASE     = 2;

// Phase-Nachricht: wir benutzen die `entries`-Slots zweckentfremdet,
// damit das Wire-Format stabil bleibt. entries[0].playerId enthält die Phase,
// entries[0].points enthält die Restzeit in Sekunden.
//   entries[0].playerId : MatchPhase (0=IDLE, 1=LOBBY, 2=ACTIVE, 3=DONE)
//   entries[0].points   : Sekunden bis zum nächsten Phasenwechsel
//   entries[0].player   : Modus-Name (z.B. "free-for-all")

// Lokaler Snapshot zum Diff-Detect
struct PlayerSnapshot {
  uint32_t playerId;
  char     player[12];
  int16_t  lastPoints;
  uint32_t lastUpdate;
};

enum MatchPhase : uint8_t {
  MATCH_IDLE   = 0,
  MATCH_LOBBY  = 1,
  MATCH_ACTIVE = 2,
  MATCH_DONE   = 3
};

struct MatchSettings {
  char     mode[24];
  uint32_t lobbySeconds;
  uint32_t matchSeconds;
};

#endif
