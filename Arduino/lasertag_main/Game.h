#ifndef LASERTAG_GAME_H
#define LASERTAG_GAME_H

#include <Arduino.h>

uint32_t getMyPlayerId();
bool isPlayerDisabled();
unsigned long getDisableTimeLeftMs();
bool handleTrigger();
bool handleIrReceiver();
void updateRespawnDisplayState(bool disabledNow, bool &wasPlayerDisabled);

#endif
