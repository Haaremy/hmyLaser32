#include "Led.h"
#include "Config.h"
#include "Globals.h"
#include "Identity.h"

void analogWriteRgb(uint8_t pin, uint8_t value) {
  if (RGB_COMMON_ANODE) analogWrite(pin, 255 - value);
  else                  analogWrite(pin, value);
}

void setRgbColor(uint8_t red, uint8_t green, uint8_t blue) {
  analogWriteRgb(RGB_RED_PIN, red);
  analogWriteRgb(RGB_GREEN_PIN, green);
  analogWriteRgb(RGB_BLUE_PIN, blue);
}

static void setColorFromU32(uint32_t c) {
  setRgbColor((uint8_t)((c >> 16) & 0xFF), (uint8_t)((c >> 8) & 0xFF), (uint8_t)(c & 0xFF));
}

void setLedNormalState() {
  setRgbColor(255, 255, 255);
}

void setLedHitState() {
  // Default: kein Blink-Status gesetzt → LED aus
  setRgbColor(0, 0, 0);
}

void applyTeamColor() {
  // Team-Modus hat Vorrang, sonst eigene Identitäts-Farbe
  bool teamsFresh = g_teamCount > 0 && (millis() - g_teamsLastUpdate) < TEAMS_TIMEOUT_MS;
  if (teamsFresh && g_myTeamIndex >= 0 && g_myTeamIndex < (int)g_teamCount) {
    setColorFromU32(g_teams[g_myTeamIndex].color);
    return;
  }
  if (identityIsAssigned()) {
    setColorFromU32(identityMyColor());
    return;
  }
  setLedNormalState();
}

// v4: 3× Blink in Schützenfarbe, dann Respawn-Dunkel.
// Wird beim Treffer (Game.cpp::handleIrReceiver) angestoßen.
void triggerHitBlink(uint32_t shooterColor) {
  g_hitBlinkColor = shooterColor;
  g_hitBlinkRemaining = 6;   // 3× an + 3× aus
  g_hitBlinkUntilMs = millis() + HIT_BLINK_INTERVAL_MS;
}

// In loop() aufrufen; treibt die Blink-Animation und schaltet danach um
// auf den HIT_DISABLE-Dunkelzustand.
void ledTickHitBlink() {
  if (g_hitBlinkRemaining <= 0) return;
  unsigned long now = millis();
  if ((long)(now - g_hitBlinkUntilMs) < 0) return;

  // 6 → 5 → 4 → 3 → 2 → 1 → 0
  // Even: ON (color), Odd: OFF (dark)
  bool turnOn = (g_hitBlinkRemaining % 2) == 0;
  if (turnOn) setColorFromU32(g_hitBlinkColor);
  else        setRgbColor(0, 0, 0);

  g_hitBlinkRemaining--;
  g_hitBlinkUntilMs = now + HIT_BLINK_INTERVAL_MS;

  if (g_hitBlinkRemaining == 0) {
    // Animation fertig → Dunkel bis Respawn
    setRgbColor(0, 0, 0);
  }
}
