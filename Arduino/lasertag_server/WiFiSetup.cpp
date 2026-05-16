#include "WiFiSetup.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include "Config.h"
#include "Globals.h"
#include "Storage.h"

static DNSServer dnsServer;
static bool kApActive = false;
static bool kMdnsActive = false;
static char kApSsid[40] = {0};

static String mdnsHostFromName(const char *name) {
  // hmylaser32-bluewolf-a4f2 (lowercase, ASCII)
  String host = String(MDNS_PREFIX) + "-" + String(name);
  host.toLowerCase();
  String clean;
  clean.reserve(host.length());
  for (size_t i = 0; i < host.length(); ++i) {
    char c = host.charAt(i);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') clean += c;
  }
  if (clean.length() == 0) clean = "hmylaser32";
  return clean;
}

static void startMdns() {
  String host = mdnsHostFromName(g_serverName);
  if (MDNS.begin(host.c_str())) {
    MDNS.addService("http", "tcp", 80);
    strlcpy(g_mdnsHost, host.c_str(), sizeof(g_mdnsHost));
    kMdnsActive = true;
    LT_LOG("mDNS: http://%s.local/", host.c_str());
  } else {
    LT_LOG("mDNS start failed");
    g_mdnsHost[0] = '\0';
  }
}

static void startCaptiveAp() {
  if (kApActive) return;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(kApSsid, sizeof(kApSsid), "%s%02X%02X", AP_SSID_PREFIX, mac[4], mac[5]);

  IPAddress ip(AP_IP[0], AP_IP[1], AP_IP[2], AP_IP[3]);
  IPAddress mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, mask);
  if (strlen(AP_PASSWORD) == 0) {
    WiFi.softAP(kApSsid, nullptr, WIFI_CHANNEL);
  } else {
    WiFi.softAP(kApSsid, AP_PASSWORD, WIFI_CHANNEL);
  }
  // DNS-Hijack für Captive-Portal-Erkennung
  dnsServer.start(53, "*", ip);
  kApActive = true;
  strlcpy(g_apSsid, kApSsid, sizeof(g_apSsid));
  LT_LOG("AP up: %s @ %s (Ch %d)", kApSsid, ip.toString().c_str(), WIFI_CHANNEL);
}

static bool tryStation() {
  String ssid = storageGetString("ssid", "");
  String psk  = storageGetString("psk",  "");
  if (ssid.length() == 0) {
    LT_LOG("No stored SSID");
    return false;
  }

  LT_LOG("STA connect to '%s'", ssid.c_str());
  WiFi.begin(ssid.c_str(), psk.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < STA_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    strlcpy(g_staIp, WiFi.localIP().toString().c_str(), sizeof(g_staIp));
    g_staChannel = WiFi.channel();
    g_wifiConnected = true;
    LT_LOG("STA connected. IP=%s Ch=%d RSSI=%d", g_staIp, g_staChannel, WiFi.RSSI());
    return true;
  }

  LT_LOG("STA connect failed");
  return false;
}

bool wifiBegin() {
  // Wichtig: AP+STA-Mode permanent. ESP-NOW läuft auf dem aktuellen Channel.
  WiFi.mode(WIFI_AP_STA);
  startCaptiveAp();

  bool sta = tryStation();
  if (sta) {
    // ESP32 kann im AP+STA-Mode nur einen Channel. Wir folgen dem STA-Channel.
    esp_wifi_set_channel(g_staChannel, WIFI_SECOND_CHAN_NONE);
    startMdns();
  } else {
    // Kein STA → AP bleibt auf WIFI_CHANNEL
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  }
  return sta;
}

void wifiForgetCredentials() {
  storageSetString("ssid", "");
  storageSetString("psk",  "");
  g_wifiConnected = false;
  g_staIp[0] = '\0';
  if (kMdnsActive) { MDNS.end(); kMdnsActive = false; }
  WiFi.disconnect(false, true);
  LT_LOG("Credentials forgotten");
}

bool wifiTryReconnect() {
  if (WiFi.status() == WL_CONNECTED) return true;
  return tryStation();
}

void wifiLoop() {
  if (kApActive) dnsServer.processNextRequest();
  static unsigned long lastTry = 0;
  if (!g_wifiConnected) {
    if (millis() - lastTry > 10000) {
      lastTry = millis();
      String ssid = storageGetString("ssid", "");
      if (ssid.length() > 0) {
        WiFi.reconnect();
        if (WiFi.status() == WL_CONNECTED) {
          strlcpy(g_staIp, WiFi.localIP().toString().c_str(), sizeof(g_staIp));
          g_staChannel = WiFi.channel();
          g_wifiConnected = true;
          esp_wifi_set_channel(g_staChannel, WIFI_SECOND_CHAN_NONE);
          if (!kMdnsActive) startMdns();
        }
      }
    }
  } else if (WiFi.status() != WL_CONNECTED) {
    g_wifiConnected = false;
  }
}

String wifiScanJson() {
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
    out += ",\"channel\":";
    out += String(WiFi.channel(i));
    out += ",\"secure\":";
    out += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "false" : "true";
    out += "}";
  }
  out += "]";
  WiFi.scanDelete();
  return out;
}
