// ============================================================================
//   hmyLaser32 — Client (v4.1)
//   v1: stand-alone IR/ESP-NOW Gossip
//   v2: + Match-Phase-Awareness via MSG_PHASE
//   v3: + Team-Mode via MSG_TEAMS
//   v4: + Auto-Identity (MAC-Aushandlung, 30 Namen + 10 Farben)
//       + Wait-Mode, Hit-Effekt, Stand-Alone-Lobby
//       + Friendly Fire, NFC-Logik (optional HAS_NFC)
//   v4.1: + Master-Election + MSG_STANDALONE-Konsens
//         + Button auf GPIO 19 (default, NFC nur als Empfehlung)
//         + Schnellere Discovery in den ersten 15s (1 s statt 5 s)
// ============================================================================

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "Config.h"
#include "Display.h"
#include "Game.h"
#include "Globals.h"
#include "Identity.h"
#include "LasertagNetwork.h"
#include "Led.h"
#include "Nfc.h"
#include "Ranking.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
IRrecv irrecvSecondary(IR_RECV_PIN_SECONDARY);
IRrecv irrecvTertiary(IR_RECV_PIN_TERTIARY);
decode_results results;
decode_results resultsSecondary;
decode_results resultsTertiary;

bool lastButtonState = HIGH;
bool lastRawButtonReading = HIGH;
unsigned long lastDebounce = 0;
unsigned long lastDisplayRefreshAt = 0;

int hitCount = 0;
int hitCountPrimary = 0;
int hitCountSecondary = 0;
int hitCountTertiary = 0;
int shotsFired = 0;
int myPoints = 0;
unsigned long lastShotAt = 0;
unsigned long playerDisabledUntil = 0;

RankingEntry ranking[MAX_PLAYERS] = {};
Message outgoing = {};
uint8_t knownPeers[MAX_PEERS][6] = {};
int peerCount = 0;
bool pendingTableBroadcast = false;
unsigned long lastTableBroadcast = 0;
volatile bool sendStatusPending = false;
volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;

static uint8_t detectEspNowChannel() {
  int bestRssi = -1000;
  uint8_t bestChannel = WIFI_CHANNEL;
  int n = WiFi.scanNetworks(false, true, false, 250);
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.startsWith(SERVER_AP_SSID_PREFIX)) continue;
    int rssi = WiFi.RSSI(i);
    int channel = WiFi.channel(i);
    if (channel > 0 && rssi > bestRssi) {
      bestRssi = rssi;
      bestChannel = (uint8_t)channel;
    }
  }
  WiFi.scanDelete();
  Serial.printf("[ESP-NOW] Channel selected: %u%s\n",
                (unsigned)bestChannel,
                bestRssi > -1000 ? " (server AP detected)" : " (fallback)");
  return bestChannel;
}

// Phase (v2/v3)
uint8_t  g_phase = PHASE_IDLE;
uint32_t g_phaseSecondsLeft = 0;
unsigned long g_phaseLastUpdate = 0;
char g_phaseMode[16] = "free-for-all";
int16_t g_hitPoints = DEFAULT_HIT_POINTS;
int16_t g_zonePoints[3] = { DEFAULT_ZONE1_POINTS, DEFAULT_ZONE2_POINTS, DEFAULT_ZONE3_POINTS };

// Teams (v3)
TeamDef g_teams[MAX_TEAMS] = {};
uint8_t g_teamCount = 0;
int8_t  g_myTeamIndex = -1;
uint32_t g_myAssignedColor = 0;
unsigned long g_teamsLastUpdate = 0;

// Hit-Blink (v4)
uint32_t g_hitBlinkColor = 0x000000;
unsigned long g_hitBlinkUntilMs = 0;
int g_hitBlinkRemaining = 0;

// Standalone-Lobby (v4 + v4.1 Master-Election)
uint8_t  g_standalonePhase = PHASE_IDLE;
unsigned long g_standaloneLobbyEnds = 0;
unsigned long g_standaloneMatchEnds = 0;
unsigned long g_standaloneLastUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);
  u8g2.begin();
  u8g2_prepare();
  u8g2.clearBuffer();
  u8g2.drawStr(0, 0, "Booting...");
  u8g2.sendBuffer();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setLedNormalState();
  irsend.begin();
  irrecv.enableIRIn();
  irrecvSecondary.enableIRIn();
  irrecvTertiary.enableIRIn();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  uint8_t espNowChannel = detectEspNowChannel();
  esp_wifi_set_channel(espNowChannel, WIFI_SECOND_CHAN_NONE);

  identityInit();
  nfcBegin();

  if (esp_now_init() != ESP_OK) { Serial.println("[ESP-NOW] Init failed"); return; }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t broadcastPeer = {};
  const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(broadcastPeer.peer_addr, broadcast, 6);
  broadcastPeer.channel = 0;
  broadcastPeer.encrypt = false;
  esp_now_add_peer(&broadcastPeer);

  Serial.println("Lasertag client v4.1 ready");
  Serial.print("[ESP-NOW] My MAC: ");
  Serial.println(WiFi.macAddress());
  updateDisplay();
  sendDiscovery();
}

void loop() {
  static unsigned long lastDiscovery = 0;
  static unsigned long lastPeriodicBroadcast = 0;
  static bool wasPlayerDisabled = false;
  static unsigned long lastLobbyTick = 0;
  static unsigned long lastStandaloneTick = 0;
  static const unsigned long bootAtMs = millis();

  identityLoop();
  nfcLoop();
  ledTickHitBlink();

  const bool disabledNow = isPlayerDisabled();
  if (!handleTrigger()) return;
  if (!handleIrReceiver()) return;

  updateRespawnDisplayState(disabledNow, wasPlayerDisabled);
  handleSendStatusLog();

  // v4.1: schnellere Discovery in den ersten 15s, danach langsamer
  unsigned long discoveryInterval =
    (millis() - bootAtMs < DISCOVERY_FAST_PHASE_MS)
      ? DISCOVERY_FAST_INTERVAL_MS
      : DISCOVERY_SLOW_INTERVAL_MS;
  if (millis() - lastDiscovery > discoveryInterval) {
    sendDiscovery();
    lastDiscovery = millis();
  }

  if (pendingTableBroadcast && millis() - lastTableBroadcast > 300) {
    broadcastRankingTable();
    broadcastPlayerState();
  }
  if (millis() - lastPeriodicBroadcast > 10000) {
    queueTableBroadcast();
    lastPeriodicBroadcast = millis();
  }
  if ((g_phase == PHASE_LOBBY || g_phase == PHASE_DISTRIBUTING) && millis() - lastLobbyTick > 1000) {
    lastLobbyTick = millis();
    updateDisplay();
  }
  if (millis() - lastStandaloneTick > 1000) {
    lastStandaloneTick = millis();
    standaloneStateTick(peerCount + 1);
    if (g_standalonePhase == PHASE_ACTIVE) {
      updateDisplay();
    }
  }
}
