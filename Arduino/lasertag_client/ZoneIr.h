#ifndef LASERTAG_ZONE_IR_H
#define LASERTAG_ZONE_IR_H

#include <Arduino.h>

struct ZoneIrFrame {
  uint8_t zone;
  uint8_t pin;
  uint32_t raw;
  uint32_t startUs;
};

void zoneIrBegin();
bool zoneIrPoll(ZoneIrFrame &frame);

#endif
