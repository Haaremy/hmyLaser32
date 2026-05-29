#ifndef LASERTAG_SERVER_TYPES_H
#define LASERTAG_SERVER_TYPES_H

#include <Arduino.h>
#include "Config.h"

// ============================================================================
// ESP-NOW Wire-Format — IDENTISCH zum Client. Bei jeder Änderung Client mit-
// migrieren! Struktur-Size darf nicht wachsen, damit alte Geräte nicht crashen
// (sie ignorieren unbekannte msgType-Werte).
// ============================================================================

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

// Message types
constexpr uint8_t MSG_DISCOVERY = 0;
constexpr uint8_t MSG_TABLE     = 1;
constexpr uint8_t MSG_PHASE     = 2;
constexpr uint8_t MSG_TEAMS     = 3;   // NEU v3
constexpr uint8_t MSG_NFC       = 4;
constexpr uint8_t MSG_STANDALONE = 5;
constexpr uint8_t MSG_PLAYER_STATE  = 6;
constexpr uint8_t MSG_PLAYER_CONFIG = 7;
constexpr uint8_t MSG_HIT_EVENT     = 8;

// MSG_PHASE slot mapping
//   entries[0].playerId : MatchPhase (0..3)
//   entries[0].points   : Sekunden bis zum nächsten Phasenwechsel
//   entries[0].player   : Modus-Name (z.B. "free-for-all" / "team")

// MSG_TEAMS slot mapping (eine RankingEntry-Slot pro Team):
//   entries[i].player     : Team-Name (12 byte)
//   entries[i].playerId   : RGB-Farbe als 0x00RRGGBB
//   entries[i].lastUpdate : Member-Bitmask (Bit n = MY_IR_COMMAND n+1 gehört zum Team)
//                           also Bit 0 = command 0x01, Bit 1 = 0x02, ..., Bit 31 = 0x20
//   entries[i].points     : 0 (reserviert)

struct PlayerSnapshot {
  uint32_t playerId;
  char     player[12];
  char     nfcToken[40];
  int16_t  lastPoints;
  uint16_t shotsFired;
  uint16_t rxHits;
  uint32_t color;
  uint8_t  teamIndex;
  uint32_t lastUpdate;
};

enum MatchPhase : uint8_t {
  MATCH_IDLE   = 0,
  MATCH_LOBBY  = 1,
  MATCH_DISTRIBUTING = 2,
  MATCH_ACTIVE = 3,
  MATCH_DONE   = 4
};

struct TeamDef {
  char     name[12];
  uint32_t color;         // 0x00RRGGBB
  uint32_t memberBits;    // Bit n = MY_IR_COMMAND (n+1) ist im Team
};

struct MatchSettings {
  char     mode[24];          // "free-for-all" | "team"
  uint32_t lobbySeconds;
  uint32_t matchSeconds;
  int16_t  hitPoints;
  int16_t  zonePoints[3];
  uint8_t  teamCount;
  TeamDef  teams[MAX_TEAMS];
};

#endif
