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
  } else {
    u8g2.drawStr(0, 0, MY_NAME);

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
