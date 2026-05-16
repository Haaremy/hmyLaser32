// ============================================================================
//   hmyLaser32 — Client (Spieler-ESP, v3)
//   v1: stand-alone IR/ESP-NOW Gossip
//   v2: + Match-Phase-Awareness via MSG_PHASE
//   v3: + Team-Mode via MSG_TEAMS (RGB-LED + OLED zeigen Team)
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
#include "LasertagNetwork.h"
#include "Led.h"
#include "Ranking.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

bool lastButtonState = HIGH;
bool lastRawButtonReading = HIGH;
unsigned long lastDebounce = 0;
unsigned long lastDisplayRefreshAt = 0;

int hitCount = 0;
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

// Phase (v2)
uint8_t  g_phase = PHASE_IDLE;
uint32_t g_phaseSecondsLeft = 0;
unsigned long g_phaseLastUpdate = 0;
char g_phaseMode[16] = "free-for-all";

// Teams (v3)
TeamDef g_teams[MAX_TEAMS] = {};
uint8_t g_teamCount = 0;
int8_t  g_myTeamIndex = -1;
unsigned long g_teamsLastUpdate = 0;

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
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  setLedNormalState();
  irsend.begin();
  irrecv.enableIRIn();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) { Serial.println("[ESP-NOW] Init failed"); return; }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t broadcastPeer = {};
  const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(broadcastPeer.peer_addr, broadcast, 6);
  broadcastPeer.channel = WIFI_CHANNEL;
  broadcastPeer.encrypt = false;
  esp_now_add_peer(&broadcastPeer);

  updateMyScore(myPoints);
  syncMyPointsFromRanking();

  Serial.println("Lasertag ESP-NOW ready (v3 team aware)");
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

  const bool disabledNow = isPlayerDisabled();
  if (!handleTrigger()) return;
  if (!handleIrReceiver()) return;

  updateRespawnDisplayState(disabledNow, wasPlayerDisabled);
  handleSendStatusLog();

  if (millis() - lastDiscovery > 5000) {
    sendDiscovery();
    lastDiscovery = millis();
  }
  if (pendingTableBroadcast && millis() - lastTableBroadcast > 300) {
    broadcastRankingTable();
  }
  if (millis() - lastPeriodicBroadcast > 10000) {
    queueTableBroadcast();
    lastPeriodicBroadcast = millis();
  }
  if (g_phase == PHASE_LOBBY && millis() - lastLobbyTick > 1000) {
    lastLobbyTick = millis();
    updateDisplay();
  }
}
