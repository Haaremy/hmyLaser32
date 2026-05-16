#include "Storage.h"
#include <Preferences.h>
#include "Config.h"

static Preferences prefs;

void storageBegin() {
  prefs.begin("laser32", false);
}

String storageGetString(const char *key, const String &fallback) {
  return prefs.getString(key, fallback);
}

void storageSetString(const char *key, const String &value) {
  prefs.putString(key, value);
}

uint32_t storageGetU32(const char *key, uint32_t fallback) {
  return prefs.getUInt(key, fallback);
}

void storageSetU32(const char *key, uint32_t value) {
  prefs.putUInt(key, value);
}

void storageClear() {
  prefs.clear();
}
