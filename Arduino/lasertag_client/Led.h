#ifndef LASERTAG_LED_H
#define LASERTAG_LED_H

#include <Arduino.h>

void setRgbColor(uint8_t red, uint8_t green, uint8_t blue);
void setLedNormalState();
void setLedHitState();
void applyTeamColor();

// v4: Hit-Effekt — 3× Blink in der Farbe des Schützen.
// Wird in loop() durch ledTickHitBlink() weitergetrieben.
void triggerHitBlink(uint32_t shooterColor);
void ledTickHitBlink();

#endif
