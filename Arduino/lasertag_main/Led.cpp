#include "Led.h"
#include "Config.h"

void analogWriteRgb(uint8_t pin, uint8_t value) {
  if (RGB_COMMON_ANODE) {
    analogWrite(pin, 255 - value);
  } else {
    analogWrite(pin, value);
  }
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
