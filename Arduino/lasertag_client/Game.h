#ifndef LASERTAG_GAME_H
#define LASERTAG_GAME_H

#include <Arduino.h>

uint32_t getMyPlayerId();
bool isPlayerDisabled();
unsigned long getDisableTimeLeftMs();
bool isShootingAllowed();
bool handleTrigger();
bool handleIrReceiver();
void updateRespawnDisplayState(bool disabledNow, bool &wasPlayerDisabled);
void resetRuntimeStatsForNewMatch();

// v4: Standalone-Lobby/Match Konsens-Timer. peerCountIncludingSelf = unique
// MAC-Adressen die der Client zur Zeit kennt (eigene zählt mit).
void standaloneStateTick(int peerCountIncludingSelf);

#endif
