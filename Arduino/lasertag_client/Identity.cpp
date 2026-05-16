#include "Identity.h"
#include <WiFi.h>
#include <cstring>
#include "Display.h"
#include "Globals.h"
#include "Led.h"

// 30 Namen — wechselbar, müssen aber synchron zwischen allen Clients sein.
const char *NAME_POOL[NAME_POOL_SIZE] = {
  "Alpha", "Bravo", "Charlie", "Delta", "Echo",
  "Foxtrot", "Golf", "Hotel", "India", "Juliet",
  "Kilo", "Lima", "Mike", "November", "Oscar",
  "Papa", "Quebec", "Romeo", "Sierra", "Tango",
  "Uniform", "Victor", "Whiskey", "Xray", "Yankee",
  "Zulu", "Apex", "Blaze", "Comet", "Drift"
};

// 10 Farben (0x00RRGGBB) — kontrastreich
const uint32_t COLOR_POOL[COLOR_POOL_SIZE] = {
  0xdc2626, // Rot
  0x2563eb, // Blau
  0x16a34a, // Grün
  0xd97706, // Orange
  0x9333ea, // Lila
  0x0891b2, // Cyan
  0xeab308, // Gelb
  0xec4899, // Pink
  0x64748b, // Grau
  0xffffff  // Weiß
};

struct KnownPeer {
  uint8_t  mac[6];
  bool     isServer;
  unsigned long lastSeen;
};

static KnownPeer kPeers[MAX_PEERS];
static int       kPeerCount = 0;
static uint8_t   kMyMac[6] = {0};
static bool      kAssigned = false;
static uint8_t   kMyIndex = 0;
static char      kMyName[16] = "Searching...";
static uint32_t  kMyColor = 0xffffff;
static unsigned long kLastPeerEvent = 0;
static unsigned long kLastRecheck = 0;
static bool      kHasServer = false;

static int macCmp(const uint8_t *a, const uint8_t *b) { return memcmp(a, b, 6); }

static int findPeer(const uint8_t *mac) {
  for (int i = 0; i < kPeerCount; i++) {
    if (memcmp(kPeers[i].mac, mac, 6) == 0) return i;
  }
  return -1;
}

static void sortPeersByMac() {
  for (int i = 0; i < kPeerCount - 1; i++) {
    for (int j = i + 1; j < kPeerCount; j++) {
      if (macCmp(kPeers[i].mac, kPeers[j].mac) > 0) {
        KnownPeer tmp = kPeers[i];
        kPeers[i] = kPeers[j];
        kPeers[j] = tmp;
      }
    }
  }
}

static void logPeers(const char *tag) {
  Serial.printf("[IDENTITY:%s] kPeerCount=%d, kAssigned=%d, myIndex=%u\n",
                tag, kPeerCount, kAssigned, kMyIndex);
  for (int i = 0; i < kPeerCount; i++) {
    Serial.printf("  [%d] %02X:%02X:%02X:%02X:%02X:%02X%s\n", i,
                  kPeers[i].mac[0], kPeers[i].mac[1], kPeers[i].mac[2],
                  kPeers[i].mac[3], kPeers[i].mac[4], kPeers[i].mac[5],
                  (memcmp(kPeers[i].mac, kMyMac, 6) == 0) ? "  <- me" : "");
  }
}

void identityInit() {
  WiFi.macAddress(kMyMac);
  Serial.printf("[IDENTITY] My MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                kMyMac[0], kMyMac[1], kMyMac[2], kMyMac[3], kMyMac[4], kMyMac[5]);

  memcpy(kPeers[0].mac, kMyMac, 6);
  kPeers[0].isServer = false;
  kPeers[0].lastSeen = millis();
  kPeerCount = 1;

  kLastPeerEvent = millis();
  kAssigned = false;
  strlcpy(kMyName, "Searching...", sizeof(kMyName));
  kMyColor = 0xffffff;
}

void identityPeerSeen(const uint8_t *mac, bool isServer) {
  int idx = findPeer(mac);
  unsigned long now = millis();
  if (idx < 0) {
    if (kPeerCount >= MAX_PEERS) return;
    memcpy(kPeers[kPeerCount].mac, mac, 6);
    kPeers[kPeerCount].isServer = isServer;
    kPeers[kPeerCount].lastSeen = now;
    kPeerCount++;
    kLastPeerEvent = now;
    Serial.printf("[IDENTITY] +peer #%d %02X:%02X:%02X:%02X:%02X:%02X (server=%d)\n",
                  kPeerCount, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], isServer);
  } else {
    kPeers[idx].lastSeen = now;
    if (isServer) kPeers[idx].isServer = true;
  }
  if (isServer) kHasServer = true;
}

static void assignIdentity() {
  sortPeersByMac();
  int idx = -1;
  for (int i = 0; i < kPeerCount; i++) {
    if (memcmp(kPeers[i].mac, kMyMac, 6) == 0) { idx = i; break; }
  }
  if (idx < 0) idx = 0;
  kMyIndex = (uint8_t)idx;
  strlcpy(kMyName, NAME_POOL[idx % NAME_POOL_SIZE], sizeof(kMyName));
  kMyColor = COLOR_POOL[idx % COLOR_POOL_SIZE];
  kAssigned = true;
  Serial.printf("[IDENTITY] *** ASSIGN *** idx=%d name=%s color=#%06lx (peers=%d)\n",
                idx, kMyName, (unsigned long)kMyColor, kPeerCount);
  logPeers("post-assign");
  applyTeamColor();
  updateDisplay();
}

void identityLoop() {
  unsigned long now = millis();

  // Wait-Phase: assign sobald ≥2 Peers (uns selbst eingeschlossen) UND
  // seit IDENTITY_WAIT_MS keine neuen Peers mehr.
  if (!kAssigned && (now - kLastPeerEvent) > IDENTITY_WAIT_MS) {
    if (kPeerCount >= 2 || kHasServer) {
      assignIdentity();
    }
  }

  // Recheck: wenn später neue Peers dazukommen, könnte sich unser Index
  // verschieben. Aber Vorsicht — wir wollen nicht ständig den Namen
  // ändern, sobald Identity assigned ist. Lass den Index stabil bleiben
  // (Spec: "weitere Spieler erhalten aufsteigend Namen").
  // Hier nur logger-output.
  if (kAssigned && (now - kLastRecheck) > 10000UL) {
    kLastRecheck = now;
    logPeers("periodic");
  }
}

const char *identityMyName()   { return kMyName; }
uint32_t    identityMyColor()  { return kMyColor; }
uint8_t     identityMyCommand(){ return (uint8_t)((kMyIndex % 254) + 1); }
bool        identityIsAssigned(){ return kAssigned; }

// === Master-Election (v4.1) ==============================================
// Der Client mit der niedrigsten bekannten MAC ist Master und sendet
// MSG_STANDALONE 1×/s.
bool identityIsLobbyMaster() {
  if (!kAssigned || kPeerCount < 2) return false;
  // Nach assignment ist kPeers[0] (sortiert) der mit kleinster MAC.
  return memcmp(kPeers[0].mac, kMyMac, 6) == 0;
}

uint8_t identityPeerCount() { return (uint8_t)kPeerCount; }
