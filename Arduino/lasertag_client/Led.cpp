#include "Led.h"
#include "Config.h"
#include "Globals.h"

void analogWriteRgb(uint8_t pin, uint8_t value) {
  if (RGB_COMMON_ANODE) analogWrite(pin, 255 - value);
  else                  analogWrite(pin, value);
}

void setRgbColor(uint8_t red, uint8_t green, uint8_t blue) {
  analogWriteRgb(RGB_RED_PIN, red);
  analogWriteRgb(RGB_GREEN_PIN, green);
  analogWriteRgb(RGB_BLUE_PIN, blue);
}

void setLedNormalState() {
  setRgbColor(255, 255, 255);
}

void setLedHitState() {
  setRgbColor(0, 0, 0);
}

void applyTeamColor() {
  bool teamsFresh = g_teamCount > 0 && (millis() - g_teamsLastUpdate) < TEAMS_TIMEOUT_MS;
  if (teamsFresh && g_myTeamIndex >= 0 && g_myTeamIndex < (int)g_teamCount) {
    uint32_t c = g_teams[g_myTeamIndex].color;
    setRgbColor((uint8_t)((c >> 16) & 0xFF), (uint8_t)((c >> 8) & 0xFF), (uint8_t)(c & 0xFF));
  } else {
    setLedNormalState();
  }
}
