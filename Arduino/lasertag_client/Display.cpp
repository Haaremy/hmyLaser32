#include "Display.h"
#include <Arduino.h>
#include "Config.h"
#include "Game.h"
#include "Globals.h"
#include "Identity.h"
#include "Ranking.h"

void u8g2_prepare() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

static const char *phaseLabel(uint8_t p) {
  switch (p) {
    case PHASE_LOBBY:  return "LOBBY";
    case PHASE_DISTRIBUTING: return "START";
    case PHASE_ACTIVE: return "ACTIVE";
    case PHASE_DONE:   return "DONE";
    default:           return "IDLE";
  }
}

static uint8_t currentPhase() {
  if (g_phaseLastUpdate != 0 && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS) {
    return g_phase == PHASE_IDLE ? PHASE_ACTIVE : g_phase;
  }
  if (g_standalonePhase != PHASE_IDLE) return g_standalonePhase;
  return PHASE_ACTIVE;
}

static const char *myTeamName() {
  bool teamsFresh = g_teamCount > 0 && (millis() - g_teamsLastUpdate) < TEAMS_TIMEOUT_MS;
  if (teamsFresh && g_myTeamIndex >= 0 && g_myTeamIndex < (int)g_teamCount) {
    return g_teams[g_myTeamIndex].name;
  }
  return nullptr;
}

void updateDisplay() {
  lastDisplayRefreshAt = millis();
  char line[24];
  u8g2.clearBuffer();

  // Wait-Mode (noch keine Identität)
  if (!identityIsAssigned()) {
    u8g2.drawStr(0, 0, "hmyLaser32");
    u8g2.drawStr(0, 18, "Suche Mitspieler...");
    u8g2.drawStr(0, 36, "Bitte warten");
    u8g2.sendBuffer();
    return;
  }

  if (isPlayerDisabled()) {
    const bool blinkOn = (millis() / 300UL) % 2 == 0;
    const unsigned long secondsLeft = getDisableTimeLeftMs() / 1000UL + 1;
    if (blinkOn) {
      u8g2.setDrawColor(1); u8g2.drawBox(0, 0, 128, 64);
      u8g2.setDrawColor(0);
      u8g2.drawFrame(2, 2, 124, 60);
      u8g2.drawFrame(6, 6, 116, 52);
      u8g2.drawStr(16, 8, "YOU GOT SHOT");
      u8g2.drawStr(34, 26, "RESPAWN");
      snprintf(line, sizeof(line), "IN %lus", secondsLeft);
      u8g2.drawStr(40, 44, line);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawFrame(0, 0, 128, 64);
      u8g2.drawFrame(3, 3, 122, 58);
      u8g2.drawFrame(6, 6, 116, 52);
      u8g2.drawStr(16, 8, "YOU GOT SHOT");
      u8g2.drawStr(34, 26, "RESPAWN");
      snprintf(line, sizeof(line), "IN %lus", secondsLeft);
      u8g2.drawStr(40, 44, line);
    }
    u8g2.sendBuffer();
    return;
  }

  uint8_t p = currentPhase();
  bool serverPhase = (g_phaseLastUpdate != 0 && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS);

  if (p == PHASE_LOBBY) {
    u8g2.drawStr(0, 0, "LOBBY");
    u8g2.drawStr(48, 0, serverPhase ? g_phaseMode : "standalone");
    const char *tn = myTeamName();
    if (tn) { snprintf(line, sizeof(line), "Team: %s", tn); u8g2.drawStr(0, 18, line); }
    else    { u8g2.drawStr(0, 18, "Warteraum"); }
    if (serverPhase) {
      u8g2.drawStr(0, 32, "Warte auf Start");
    } else {
      long left = (long)(g_standaloneLobbyEnds - millis()) / 1000L;
      if (left < 0) left = 0;
      snprintf(line, sizeof(line), "Start in %lds", left);
      u8g2.drawStr(0, 32, line);
    }
    u8g2.drawStr(0, 50, identityMyName());
    u8g2.sendBuffer();
    return;
  }

  if (p == PHASE_DISTRIBUTING) {
    u8g2.drawStr(0, 0, "START");
    u8g2.drawStr(48, 0, serverPhase ? g_phaseMode : "standalone");
    const char *tn = myTeamName();
    if (tn) { snprintf(line, sizeof(line), "Team: %s", tn); u8g2.drawStr(0, 18, line); }
    else    { u8g2.drawStr(0, 18, "Verteilen..."); }
    long left = serverPhase
      ? (long)g_phaseSecondsLeft - (long)((millis() - g_phaseLastUpdate) / 1000UL)
      : (long)(g_standaloneLobbyEnds - millis()) / 1000L;
    if (left < 0) left = 0;
    snprintf(line, sizeof(line), "Start in %lds", left);
    u8g2.drawStr(0, 32, line);
    u8g2.drawStr(0, 50, identityMyName());
    u8g2.sendBuffer();
    return;
  }

  if (p == PHASE_DONE) {
    u8g2.drawStr(0, 0, "MATCH ENDED");
    snprintf(line, sizeof(line), "Pts: %d", myPoints);
    u8g2.drawStr(0, 18, line);
    snprintf(line, sizeof(line), "Rank: %d", getMyRank());
    u8g2.drawStr(0, 32, line);
    u8g2.drawStr(0, 50, identityMyName());
    u8g2.sendBuffer();
    return;
  }

  // ACTIVE / IDLE
  u8g2.drawStr(0, 0, identityMyName());
  u8g2.drawStr(86, 0, phaseLabel(p));
  const char *tn = myTeamName();
  if (tn) {
    snprintf(line, sizeof(line), "Team: %s", tn); u8g2.drawStr(0, 14, line);
    snprintf(line, sizeof(line), "Pts: %d", myPoints); u8g2.drawStr(0, 28, line);
    snprintf(line, sizeof(line), "H:%d 1:%d 2:%d 3:%d", hitCount, hitCountPrimary, hitCountSecondary, hitCountTertiary); u8g2.drawStr(0, 42, line);
    snprintf(line, sizeof(line), "Rank: %d", getMyRank()); u8g2.drawStr(0, 56, line);
  } else {
    snprintf(line, sizeof(line), "Pts: %d", myPoints); u8g2.drawStr(0, 14, line);
    snprintf(line, sizeof(line), "H:%d 1:%d 2:%d 3:%d", hitCount, hitCountPrimary, hitCountSecondary, hitCountTertiary); u8g2.drawStr(0, 28, line);
    snprintf(line, sizeof(line), "Shots: %d", shotsFired); u8g2.drawStr(0, 42, line);
    snprintf(line, sizeof(line), "Rank: %d", getMyRank()); u8g2.drawStr(0, 56, line);
  }
  u8g2.sendBuffer();
}
