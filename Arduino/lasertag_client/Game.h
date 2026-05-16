#ifndef LASERTAG_GAME_H
#define LASERTAG_GAME_H

#include <Arduino.h>

uint32_t getMyPlayerId();
bool isPlayerDisabled();
unsigned long getDisableTimeLeftMs();
bool isShootingAllowed();   // NEU v2: nur in ACTIVE-Phase true (Stand-Alone: immer true)
bool handleTrigger();
bool handleIrReceiver();
void updateRespawnDisplayState(bool disabledNow, bool &wasPlayerDisabled);

#endif
