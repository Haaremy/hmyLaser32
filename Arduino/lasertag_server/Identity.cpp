#include "Identity.h"
#include <WiFi.h>
#include <esp_random.h>
#include "Config.h"
#include "Globals.h"
#include "Storage.h"

// Wortliste für menschenlesbare Servernamen. Erstes Wort = "Adjektiv",
// zweites Wort = "Nomen". Mit MAC-Suffix kollisionssicher.
static const char *kAdjectives[] = {
  "Red", "Blue", "Green", "Black", "White", "Gold", "Silver", "Crimson",
  "Steel", "Neon", "Cyber", "Quick", "Sharp", "Bold", "Storm", "Frost"
};
static const char *kNouns[] = {
  "Wolf", "Hawk", "Cobra", "Tiger", "Falcon", "Lynx", "Viper", "Eagle",
  "Bear", "Shark", "Panther", "Raven", "Fox", "Drake", "Phoenix", "Boar"
};

static String randomServerName() {
  const char *adj = kAdjectives[esp_random() % (sizeof(kAdjectives) / sizeof(kAdjectives[0]))];
  const char *nn  = kNouns[esp_random() % (sizeof(kNouns) / sizeof(kNouns[0]))];
  // letzte 2 MAC-Bytes als hex
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char suffix[6];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  String name = String(adj) + String(nn) + "-" + String(suffix);
  if (name.length() >= IDENTITY_NAME_BYTES) {
    name = name.substring(0, IDENTITY_NAME_BYTES - 1);
  }
  return name;
}

static String randomPin() {
  // 4-stellig, 1000..9999 (führende 0 ausschließen für klare Eingabe)
  uint32_t v = 1000 + (esp_random() % 9000);
  return String(v);
}

void identityLoadOrCreate() {
  String name = storageGetString("name", "");
  String pin  = storageGetString("pin", "");

  if (name.length() == 0) {
    name = randomServerName();
    storageSetString("name", name);
    LT_LOG("Generated new name: %s", name.c_str());
  }
  if (pin.length() == 0) {
    pin = randomPin();
    storageSetString("pin", pin);
    LT_LOG("Generated new PIN: %s", pin.c_str());
  }

  strlcpy(g_serverName, name.c_str(), sizeof(g_serverName));
  strlcpy(g_serverPin,  pin.c_str(),  sizeof(g_serverPin));
  LT_LOG("Identity: %s / PIN %s", g_serverName, g_serverPin);
}

void identityRegenerate() {
  String name = randomServerName();
  String pin  = randomPin();
  storageSetString("name", name);
  storageSetString("pin",  pin);
  strlcpy(g_serverName, name.c_str(), sizeof(g_serverName));
  strlcpy(g_serverPin,  pin.c_str(),  sizeof(g_serverPin));
  LT_LOG("Regenerated identity: %s / PIN %s", g_serverName, g_serverPin);
}
