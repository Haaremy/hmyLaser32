#include "WiFiSetup.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include "Config.h"
#include "Globals.h"
#include "Storage.h"

static DNSServer dnsServer;
static bool kApActive = false;

bool wifiTryStation() {
  String ssid = storageGetString("ssid", "");
  String psk  = storageGetString("psk",  "");
  if (ssid.length() == 0) {
    LT_LOG("No stored SSID — staying in setup mode");
    return false;
  }

  LT_LOG("Trying STA connect to '%s'", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.begin(ssid.c_str(), psk.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < STA_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    LT_LOG("Connected. IP=%s RSSI=%d", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    // ESP-NOW braucht festen Kanal — den von WiFi.STA übernehmen
    g_wifiConnected = true;
    return true;
  }

  LT_LOG("STA connect failed");
  WiFi.disconnect(true, true);
  return false;
}

void wifiStartCaptive() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[40];
  snprintf(ssid, sizeof(ssid), "%s%02X%02X", AP_SSID_PREFIX, mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  IPAddress ip(AP_IP[0], AP_IP[1], AP_IP[2], AP_IP[3]);
  IPAddress mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, mask);
  // Channel = WIFI_CHANNEL für ESP-NOW Co-Existenz
  if (strlen(AP_PASSWORD) == 0) {
    WiFi.softAP(ssid, nullptr, WIFI_CHANNEL);
  } else {
    WiFi.softAP(ssid, AP_PASSWORD, WIFI_CHANNEL);
  }
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // DNS-Hijack: alle Anfragen → unsere IP (Captive-Portal-Erkennung in den
  // Betriebssystemen)
  dnsServer.start(53, "*", ip);
  kApActive = true;
  g_wifiConnected = false;

  LT_LOG("Captive Portal up: SSID=%s IP=%s", ssid, ip.toString().c_str());
}

bool wifiSwitchToStationFromStorage() {
  if (kApActive) {
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    kApActive = false;
  }
  return wifiTryStation();
}

void wifiLoop() {
  if (kApActive) {
    dnsServer.processNextRequest();
    return;
  }
  // STA-Watchdog: bei Drop → Reconnect
  if (WiFi.status() != WL_CONNECTED) {
    g_wifiConnected = false;
    static unsigned long lastTry = 0;
    if (millis() - lastTry > 10000) {
      lastTry = millis();
      WiFi.reconnect();
    }
  } else {
    g_wifiConnected = true;
  }
}

String wifiScanJson() {
  // synchron scannen — kann 2-3 Sek dauern; akzeptabel im AP-Mode
  int n = WiFi.scanNetworks(false, true, false, 250);
  String out = "[";
  for (int i = 0; i < n; i++) {
    if (i) out += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    out += "{\"ssid\":\"";
    out += ssid;
    out += "\",\"rssi\":";
    out += String(WiFi.RSSI(i));
    out += ",\"secure\":";
    out += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "false" : "true";
    out += "}";
  }
  out += "]";
  WiFi.scanDelete();
  return out;
}
