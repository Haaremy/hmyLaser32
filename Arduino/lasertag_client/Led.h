#ifndef LASERTAG_LED_H
#define LASERTAG_LED_H

#include <Arduino.h>

void setRgbColor(uint8_t red, uint8_t green, uint8_t blue);
void setLedNormalState();
void setLedHitState();

#endif
