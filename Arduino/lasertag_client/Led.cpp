#include "Led.h"
#include "Config.h"
#include "Globals.h"
#include "Identity.h"
#include <FastLED.h>

static CRGB kLeds[LED_COUNT];
static bool kFastLedInited = false;

// Init wird einmal bei der ersten Verwendung gerufen (lazy).
static void ensureInit() {
  if (kFastLedInited) return;
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(kLeds, LED_COUNT);
  FastLED.setBrightness(LED_BRIGHTNESS);
  fill_solid(kLeds, LED_COUNT, CRGB::Black);
  FastLED.show();
  kFastLedInited = true;
}

static void setStripColor(uint8_t red, uint8_t green, uint8_t blue) {
  ensureInit();
  fill_solid(kLeds, LED_COUNT, CRGB(red, green, blue));
  FastLED.show();
}

static void setColorFromU32(uint32_t c) {
  setStripColor((uint8_t)((c >> 16) & 0xFF),
                (uint8_t)((c >> 8) & 0xFF),
                (uint8_t)(c & 0xFF));
}

void setLedNormalState() {
  setStripColor(255, 255, 255);
}

void setLedHitState() {
  setStripColor(0, 0, 0);
}

void applyTeamColor() {
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
void triggerHitBlink(uint32_t shooterColor) {
  g_hitBlinkColor = shooterColor;
  g_hitBlinkRemaining = 6;     // 3× an + 3× aus
  g_hitBlinkUntilMs = millis() + HIT_BLINK_INTERVAL_MS;
}

void ledTickHitBlink() {
  if (g_hitBlinkRemaining <= 0) return;
  unsigned long now = millis();
  if ((long)(now - g_hitBlinkUntilMs) < 0) return;

  bool turnOn = (g_hitBlinkRemaining % 2) == 0;
  if (turnOn) setColorFromU32(g_hitBlinkColor);
  else        setStripColor(0, 0, 0);

  g_hitBlinkRemaining--;
  g_hitBlinkUntilMs = now + HIT_BLINK_INTERVAL_MS;

  if (g_hitBlinkRemaining == 0) {
    setStripColor(0, 0, 0);
  }
}
