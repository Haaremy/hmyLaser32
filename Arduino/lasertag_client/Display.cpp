#include "Display.h"
#include <Arduino.h>
#include "Config.h"
#include "Game.h"
#include "Globals.h"
#include "Ranking.h"

void u8g2_prepare() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

static const char *phaseLabel() {
  switch (g_phase) {
    case PHASE_LOBBY:  return "LOBBY";
    case PHASE_ACTIVE: return "ACTIVE";
    case PHASE_DONE:   return "DONE";
    default:           return "IDLE";
  }
}

void updateDisplay() {
  lastDisplayRefreshAt = millis();

  Serial.print("[STATS] Shots: ");
  Serial.print(shotsFired);
  Serial.print(" Hits received: ");
  Serial.print(hitCount);
  Serial.print(" Points: ");
  Serial.print(myPoints);
  Serial.print(" Rank: ");
  Serial.println(getMyRank());

  char line[24];
  u8g2.clearBuffer();

  if (isPlayerDisabled()) {
    const bool blinkOn = (millis() / 300UL) % 2 == 0;
    const unsigned long secondsLeft = getDisableTimeLeftMs() / 1000UL + 1;
    if (blinkOn) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, 0, 128, 64);
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
  } else if (g_phase == PHASE_LOBBY && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS) {
    // Lobby-Phase: Countdown anzeigen
    u8g2.drawStr(0, 0, "LOBBY");
    u8g2.drawStr(48, 0, g_phaseMode);
    u8g2.drawStr(0, 18, "Verteilen...");
    long left = (long)g_phaseSecondsLeft - (long)((millis() - g_phaseLastUpdate) / 1000UL);
    if (left < 0) left = 0;
    snprintf(line, sizeof(line), "Start in %lds", left);
    u8g2.drawStr(0, 32, line);
    u8g2.drawStr(0, 50, MY_NAME);
  } else if (g_phase == PHASE_DONE && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS) {
    u8g2.drawStr(0, 0, "MATCH ENDED");
    snprintf(line, sizeof(line), "Pts: %d", myPoints);
    u8g2.drawStr(0, 18, line);
    snprintf(line, sizeof(line), "Rank: %d", getMyRank());
    u8g2.drawStr(0, 32, line);
    u8g2.drawStr(0, 50, MY_NAME);
  } else {
    // ACTIVE oder Stand-Alone (kein Server)
    u8g2.drawStr(0, 0, MY_NAME);
    u8g2.drawStr(86, 0, phaseLabel());

    snprintf(line, sizeof(line), "Pts: %d", myPoints);
    u8g2.drawStr(0, 14, line);

    snprintf(line, sizeof(line), "Hits rx: %d", hitCount);
    u8g2.drawStr(0, 28, line);

    snprintf(line, sizeof(line), "Shots: %d", shotsFired);
    u8g2.drawStr(0, 42, line);

    int rank = getMyRank();
    snprintf(line, sizeof(line), "Rank: %d", rank);
    u8g2.drawStr(0, 56, line);
  }

  u8g2.sendBuffer();
}
