#ifndef LASERTAG_SERVER_STORAGE_H
#define LASERTAG_SERVER_STORAGE_H

#include <Arduino.h>
#include "Types.h"

// NVS-Wrapper auf Preferences-Lib.
// Namespace "laser32" enthält:
//   - "name"        (String)  Servername
//   - "pin"         (String)  PIN
//   - "ssid"        (String)  WLAN-SSID
//   - "psk"         (String)  WLAN-Passwort
//   - "mode"        (String)  Match-Modus
//   - "lobby"       (uint32)  Lobby-Sekunden
//   - "match"       (uint32)  Match-Sekunden

void   storageBegin();
String storageGetString(const char *key, const String &fallback = "");
void   storageSetString(const char *key, const String &value);
uint32_t storageGetU32(const char *key, uint32_t fallback);
void     storageSetU32(const char *key, uint32_t value);
void   storageClear();

#endif
