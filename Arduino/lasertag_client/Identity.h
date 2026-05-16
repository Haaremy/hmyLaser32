#ifndef LASERTAG_CLIENT_IDENTITY_H
#define LASERTAG_CLIENT_IDENTITY_H

#include <Arduino.h>
#include "Config.h"

constexpr int NAME_POOL_SIZE = 30;
constexpr int COLOR_POOL_SIZE = 10;

extern const char *NAME_POOL[NAME_POOL_SIZE];
extern const uint32_t COLOR_POOL[COLOR_POOL_SIZE];

void identityInit();
void identityLoop();

const char *identityMyName();
uint32_t    identityMyColor();
uint8_t     identityMyCommand();
bool        identityIsAssigned();

void identityPeerSeen(const uint8_t *mac, bool isServer);

// v4.1 — Master-Election für Standalone-Lobby
bool    identityIsLobbyMaster();
uint8_t identityPeerCount();

#endif
