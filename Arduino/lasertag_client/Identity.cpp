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
static int kPeerCount = 0;
static uint8_t kMyMac[6] = {0};
static bool kAssigned = false;
static uint8_t kMyIndex = 0;            // Index in NAME_POOL / COLOR_POOL
static char kMyName[16] = "Searching...";
static uint32_t kMyColor = 0xffffff;
static unsigned long kLastPeerEvent = 0;
static unsigned long kLastRecheck = 0;
static bool kHasServer = false;

static int macCmp(const uint8_t *a, const uint8_t *b) {
  return memcmp(a, b, 6);
}

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

void identityInit() {
  WiFi.macAddress(kMyMac);
  Serial.printf("[IDENTITY] My MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                kMyMac[0], kMyMac[1], kMyMac[2], kMyMac[3], kMyMac[4], kMyMac[5]);

  // Eigene MAC als ersten "Peer" eintragen — wir nehmen am Aushandlungs-
  // ranking teil
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
    kLastPeerEvent = now;     // reset timer
    Serial.printf("[IDENTITY] New peer (count=%d), reset wait-timer\n", kPeerCount);
  } else {
    kPeers[idx].lastSeen = now;
    if (isServer) kPeers[idx].isServer = true;
  }
  if (isServer) kHasServer = true;
}

static void assignIdentity() {
  // Server-Mode: wir warten auf MSG_TEAMS/MSG_PHASE und übernehmen die
  // Server-Zuweisung. Aktuell vergeben wir trotzdem eine vorläufige
  // Identität, die der Server dann via MSG_TEAMS überschreiben kann.
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
  Serial.printf("[IDENTITY] Assigned: idx=%d name=%s color=#%06lx (peers=%d)\n",
                idx, kMyName, (unsigned long)kMyColor, kPeerCount);
  applyTeamColor();   // LED auf Farbe setzen
  updateDisplay();
}

void identityLoop() {
  unsigned long now = millis();
  // Wenn wir noch nicht assigned sind und seit IDENTITY_WAIT_MS Stille:
  if (!kAssigned && now - kLastPeerEvent > IDENTITY_WAIT_MS) {
    // Wenn nur wir selbst auf der Liste → Wait-Mode bleibt, kein Assign
    // (Spec: "Wenn nur ein Spieler ohne Server in der Lobby ist, soll er
    //  in einen warte/suche Modus gehen bis der nächste Spieler auftaucht")
    if (kPeerCount > 1 || kHasServer) {
      assignIdentity();
    }
  }
  // Periodischer Recheck: wenn neue Peers später kommen, neu sortieren +
  // ggf. eigenen Index updaten
  if (kAssigned && now - kLastRecheck > IDENTITY_RECHECK_MS) {
    kLastRecheck = now;
    uint8_t prev = kMyIndex;
    sortPeersByMac();
    for (int i = 0; i < kPeerCount; i++) {
      if (memcmp(kPeers[i].mac, kMyMac, 6) == 0) { kMyIndex = (uint8_t)i; break; }
    }
    if (kMyIndex != prev) {
      strlcpy(kMyName, NAME_POOL[kMyIndex % NAME_POOL_SIZE], sizeof(kMyName));
      kMyColor = COLOR_POOL[kMyIndex % COLOR_POOL_SIZE];
      Serial.printf("[IDENTITY] Re-indexed to %u (%s)\n", kMyIndex, kMyName);
      applyTeamColor();
      updateDisplay();
    }
  }
}

const char *identityMyName() { return kMyName; }
uint32_t    identityMyColor() { return kMyColor; }
uint8_t     identityMyCommand() { return (uint8_t)((kMyIndex % 254) + 1); }
bool        identityIsAssigned() { return kAssigned; }
