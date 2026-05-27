#include "Match.h"
#include <cstring>
#include "Bridge.h"
#include "Config.h"
#include "EspNow.h"
#include "Globals.h"
#include "Storage.h"

void matchLoadSettings() {
  String mode = storageGetString("mode", DEFAULT_MATCH_MODE);
  uint32_t lobby = storageGetU32("lobby", DEFAULT_LOBBY_SECONDS);
  uint32_t mlen  = storageGetU32("match", DEFAULT_MATCH_SECONDS);
  uint32_t points = storageGetU32("hitpts", DEFAULT_HIT_POINTS);
  strlcpy(g_settings.mode, mode.c_str(), sizeof(g_settings.mode));
  g_settings.lobbySeconds = lobby;
  g_settings.matchSeconds = mlen;
  g_settings.hitPoints = (int16_t)points;
  // teams werden NICHT aus NVS persistiert — sie kommen via Settings-Pull
  // vom Webservice (Source of Truth). Lokale Änderungen sind ephemer.
  g_settings.teamCount = 0;
  memset(g_settings.teams, 0, sizeof(g_settings.teams));
  LT_LOG("Settings: mode=%s lobby=%lu match=%lu hitPoints=%d",
         g_settings.mode, (unsigned long)g_settings.lobbySeconds,
         (unsigned long)g_settings.matchSeconds, (int)g_settings.hitPoints);
}

void matchSaveSettings() {
  storageSetString("mode", g_settings.mode);
  storageSetU32("lobby", g_settings.lobbySeconds);
  storageSetU32("match", g_settings.matchSeconds);
  storageSetU32("hitpts", (uint32_t)g_settings.hitPoints);
}

bool matchStart() {
  if (g_matchPhase != MATCH_IDLE && g_matchPhase != MATCH_DONE) {
    LT_LOG("Already running (phase=%d)", g_matchPhase);
    return false;
  }
  memset(g_snapshots, 0, sizeof(g_snapshots));
  g_matchPhase = MATCH_LOBBY;
  g_lobbyEndsAtMs = millis() + g_settings.lobbySeconds * 1000UL;
  LT_LOG("Lobby phase started (%lu s)", (unsigned long)g_settings.lobbySeconds);
  espNowBroadcastTeams();   // Teams sofort an Clients
  espNowBroadcastPlayerConfig();
  espNowBroadcastPhase();   // Phase sofort
  return true;
}

bool matchEnd() {
  if (g_matchPhase == MATCH_IDLE) return true;
  EndPlayer pls[MAX_PLAYERS];
  static char nfcBuf[MAX_PLAYERS][20];
  int count = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (g_snapshots[i].player[0] == '\0') continue;
    snprintf(nfcBuf[count], sizeof(nfcBuf[count]), "nec:%08lx",
             (unsigned long)g_snapshots[i].playerId);
    pls[count].nfc    = nfcBuf[count];
    pls[count].team   = nullptr;
    pls[count].hits   = g_settings.hitPoints > 0 ? g_snapshots[i].lastPoints / g_settings.hitPoints : 0;
    pls[count].deaths = g_snapshots[i].rxHits;
    pls[count].points = g_snapshots[i].lastPoints;
    count++;
  }
  bool ok = bridgeEndMatch(pls, count);
  g_matchPhase = MATCH_DONE;
  g_currentMatchId[0] = '\0';
  espNowBroadcastPhase();
  return ok;
}

bool matchAbort() {
  g_matchPhase = MATCH_IDLE;
  g_currentMatchId[0] = '\0';
  memset(g_snapshots, 0, sizeof(g_snapshots));
  espNowBroadcastPhase();
  LT_LOG("Match aborted");
  return true;
}

void matchLoop() {
  static unsigned long lastPhaseCast = 0;
  static unsigned long lastTeamsCast = 0;
  static MatchPhase    lastSent = (MatchPhase)0xFF;

  if (g_matchPhase == MATCH_LOBBY) {
    if ((long)(millis() - g_lobbyEndsAtMs) >= 0) {
      if (bridgeStartMatch()) {
        g_matchPhase = MATCH_ACTIVE;
        g_matchEndsAtMs = millis() + g_settings.matchSeconds * 1000UL;
        LT_LOG("Match active. matchId=%s end in %lu s",
               g_currentMatchId, (unsigned long)g_settings.matchSeconds);
        espNowBroadcastPhase();
      } else {
        LT_LOG("bridgeStartMatch failed — retry in 3s");
        g_lobbyEndsAtMs = millis() + 3000;
      }
    }
  } else if (g_matchPhase == MATCH_ACTIVE) {
    if ((long)(millis() - g_matchEndsAtMs) >= 0) {
      LT_LOG("Match duration over — ending");
      matchEnd();
    }
  }

  if (millis() - lastPhaseCast > PHASE_BROADCAST_MS || lastSent != g_matchPhase) {
    espNowBroadcastPhase();
    lastPhaseCast = millis();
    lastSent = g_matchPhase;
  }

  // Teams re-broadcasten während Lobby + Active (damit Clients, die später
  // anschalten, das Team noch erfahren)
  if (g_settings.teamCount > 0 &&
      (g_matchPhase == MATCH_LOBBY || g_matchPhase == MATCH_ACTIVE) &&
      millis() - lastTeamsCast > TEAMS_BROADCAST_MS) {
    espNowBroadcastTeams();
    espNowBroadcastPlayerConfig();
    lastTeamsCast = millis();
  }
}
