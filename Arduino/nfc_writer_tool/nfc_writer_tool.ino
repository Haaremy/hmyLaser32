#include <Arduino.h>
#include <DNSServer.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <MFRC522.h>
#include "Config.h"

enum WriterState : uint8_t {
  STATE_INPUT = 0,
  STATE_CONFIRM = 1,
  STATE_WAIT_CARD = 2,
  STATE_READY_TO_WRITE = 3,
  STATE_WRITING = 4,
  STATE_SUCCESS = 5,
  STATE_ERROR = 6
};

static DNSServer dnsServer;
static WebServer server(80);
static MFRC522 nfc(NFC_SS_PIN, NFC_RST_PIN);

static WriterState state = STATE_INPUT;
static String pendingContent;
static String lastReadContent;
static String lastUid;
static String lastError;
static bool cardReady = false;

static String htmlEscape(const String &s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else out += c;
  }
  return out;
}

static String uidToString() {
  String uid;
  for (byte i = 0; i < nfc.uid.size; i++) {
    if (nfc.uid.uidByte[i] < 0x10) uid += '0';
    uid += String(nfc.uid.uidByte[i], HEX);
    if (i + 1 < nfc.uid.size) uid += ':';
  }
  uid.toUpperCase();
  return uid;
}

static void stopCard() {
  nfc.PICC_HaltA();
  nfc.PCD_StopCrypto1();
}

static bool authenticateBlock(byte block, MFRC522::MIFARE_Key &key) {
  MFRC522::StatusCode status = nfc.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(nfc.uid));
  if (status != MFRC522::STATUS_OK) {
    lastError = String("Auth failed on block ") + block + ": " + nfc.GetStatusCodeName(status);
    return false;
  }
  return true;
}

static bool readCardContent(String &out) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  char raw[NFC_MAX_CONTENT_BYTES + 1] = {};
  size_t pos = 0;
  for (size_t i = 0; i < NFC_DATA_BLOCK_COUNT; i++) {
    byte block = NFC_DATA_BLOCKS[i];
    byte buffer[18] = {};
    byte size = sizeof(buffer);
    if (!authenticateBlock(block, key)) return false;
    MFRC522::StatusCode status = nfc.MIFARE_Read(block, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      lastError = String("Read failed on block ") + block + ": " + nfc.GetStatusCodeName(status);
      return false;
    }
    for (byte j = 0; j < 16 && pos < NFC_MAX_CONTENT_BYTES; j++) {
      if (buffer[j] == 0) {
        raw[pos] = '\0';
        out = String(raw);
        return true;
      }
      raw[pos++] = (char)buffer[j];
    }
  }
  raw[pos] = '\0';
  out = String(raw);
  return true;
}

static bool writeCardContent(const String &content) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  byte bytes[NFC_MAX_CONTENT_BYTES] = {};
  size_t n = content.length();
  if (n >= NFC_MAX_CONTENT_BYTES) n = NFC_MAX_CONTENT_BYTES - 1;
  memcpy(bytes, content.c_str(), n);

  for (size_t i = 0; i < NFC_DATA_BLOCK_COUNT; i++) {
    byte block = NFC_DATA_BLOCKS[i];
    if (!authenticateBlock(block, key)) return false;
    MFRC522::StatusCode status = nfc.MIFARE_Write(block, bytes + (i * 16), 16);
    if (status != MFRC522::STATUS_OK) {
      lastError = String("Write failed on block ") + block + ": " + nfc.GetStatusCodeName(status);
      return false;
    }
  }
  return true;
}

static bool pollCard() {
  if (!nfc.PICC_IsNewCardPresent()) return false;
  if (!nfc.PICC_ReadCardSerial()) return false;
  lastUid = uidToString();
  return true;
}

static const char *stateLabel() {
  switch (state) {
    case STATE_CONFIRM: return "Bestaetigen";
    case STATE_WAIT_CARD: return "Karte aufhalten";
    case STATE_READY_TO_WRITE: return "Schreiben bestaetigen";
    case STATE_WRITING: return "Schreibe";
    case STATE_SUCCESS: return "Validiert";
    case STATE_ERROR: return "Fehler";
    default: return "Inhalt einfuegen";
  }
}

static String stepClass(uint8_t step) {
  uint8_t current = 1;
  if (state == STATE_CONFIRM) current = 2;
  else if (state == STATE_WAIT_CARD) current = 3;
  else if (state == STATE_READY_TO_WRITE || state == STATE_WRITING) current = 4;
  else if (state == STATE_SUCCESS || state == STATE_ERROR) current = 5;
  if (step < current) return "done";
  if (step == current) return "active";
  return "";
}

static void renderPage() {
  String html;
  html.reserve(9000);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>hmyLaser32 NFC Writer</title><style>"
            "body{margin:0;background:#101418;color:#eef2f7;font-family:system-ui,sans-serif;line-height:1.45}"
            "main{max-width:760px;margin:0 auto;padding:1rem}h1{font-size:1.35rem;margin:.2rem 0 1rem}"
            ".card{background:#1b242e;border:1px solid #334155;border-radius:8px;padding:1rem;margin-bottom:1rem}"
            ".steps{display:grid;gap:.45rem;margin-bottom:1rem}.step{display:flex;gap:.55rem;align-items:center;color:#94a3b8}"
            ".dot{width:1.6rem;height:1.6rem;border-radius:999px;background:#334155;display:grid;place-items:center;font-weight:800;color:#cbd5e1}"
            ".step.active{color:#f8fafc}.step.active .dot{background:#2563eb;color:white}.step.done .dot{background:#16a34a;color:white}"
            "textarea,input,button{width:100%;box-sizing:border-box;border-radius:6px;border:1px solid #475569;background:#0f172a;color:#f8fafc;font:inherit;padding:.75rem}"
            "textarea{min-height:8rem;resize:vertical}button{background:#2563eb;border-color:#2563eb;font-weight:800;cursor:pointer;margin-top:.7rem}"
            "button.secondary{background:#334155;border-color:#475569}.status{font-family:ui-monospace,monospace;word-break:break-all;background:#0f172a;padding:.7rem;border-radius:6px}"
            ".err{border-color:#dc2626;color:#fecaca}.ok{border-color:#16a34a;color:#bbf7d0}small{color:#94a3b8}"
            "</style></head><body><main><h1>hmyLaser32 NFC Writer</h1>");

  html += F("<div class='steps'>");
  const char *labels[] = {
    "Inhalt einfuegen",
    "Bestaetigen",
    "Auf NFC-Karte warten",
    "Schreiben bestaetigen",
    "Lesen und validieren"
  };
  for (uint8_t i = 1; i <= 5; i++) {
    html += "<div class='step " + stepClass(i) + "'><div class='dot'>" + String(i) + "</div><div>" + labels[i - 1] + "</div></div>";
  }
  html += F("</div>");

  html += "<div class='card'><strong>Status:</strong> ";
  html += stateLabel();
  html += F("</div>");

  if (state == STATE_INPUT) {
    html += F("<form method='POST' action='/content' class='card'><label>Karteninhalt</label>"
              "<textarea name='content' autocomplete='off' autocapitalize='off' spellcheck='false' placeholder='username|uuid' required maxlength='79'></textarea>"
              "<small>Maximal 79 Zeichen. Erwartetes laser32-Format: username|uuid.</small>"
              "<button type='submit'>Weiter</button></form>");
  } else {
    html += "<div class='card'><div class='status'>";
    html += htmlEscape(pendingContent);
    html += F("</div></div>");
  }

  if (state == STATE_CONFIRM) {
    html += F("<form method='POST' action='/confirm' class='card'><button type='submit'>Inhalt bestaetigen</button></form>");
  } else if (state == STATE_WAIT_CARD) {
    html += F("<div class='card'><p>Bitte NFC-Karte an den RC522 halten.</p><script>setTimeout(()=>location.reload(),900)</script></div>");
  } else if (state == STATE_READY_TO_WRITE) {
    html += "<form method='POST' action='/write' class='card'><p>Karte erkannt: <span class='status'>";
    html += htmlEscape(lastUid);
    html += F("</span></p><button type='submit'>Auf diese Karte schreiben</button></form>");
  } else if (state == STATE_SUCCESS) {
    html += "<div class='card ok'><strong>Schreibvorgang validiert.</strong><div class='status'>";
    html += htmlEscape(lastReadContent);
    html += F("</div></div>");
  } else if (state == STATE_ERROR) {
    html += "<div class='card err'><strong>Fehler</strong><p>";
    html += htmlEscape(lastError);
    html += F("</p></div>");
  }

  html += F("<form method='POST' action='/reset' class='card'><button class='secondary' type='submit'>Neu starten</button></form>"
            "</main></body></html>");
  server.send(200, "text/html", html);
}

static void redirectRoot() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

static void handleContent() {
  String value = server.arg("content");
  value.trim();
  if (value.length() == 0 || value.length() >= NFC_MAX_CONTENT_BYTES) {
    lastError = "Inhalt fehlt oder ist zu lang.";
    state = STATE_ERROR;
    redirectRoot();
    return;
  }
  if (value.indexOf('|') < 1) {
    lastError = "Format muss username|uuid sein.";
    state = STATE_ERROR;
    redirectRoot();
    return;
  }
  pendingContent = value;
  lastReadContent = "";
  lastUid = "";
  cardReady = false;
  state = STATE_CONFIRM;
  redirectRoot();
}

static void handleConfirm() {
  if (pendingContent.length() == 0) state = STATE_INPUT;
  else state = STATE_WAIT_CARD;
  redirectRoot();
}

static void handleWrite() {
  if (!cardReady) {
    lastError = "Keine Karte bereit.";
    state = STATE_ERROR;
    redirectRoot();
    return;
  }
  state = STATE_WRITING;
  if (!writeCardContent(pendingContent)) {
    stopCard();
    state = STATE_ERROR;
    redirectRoot();
    return;
  }
  String readBack;
  if (!readCardContent(readBack)) {
    stopCard();
    state = STATE_ERROR;
    redirectRoot();
    return;
  }
  stopCard();
  lastReadContent = readBack;
  state = (readBack == pendingContent) ? STATE_SUCCESS : STATE_ERROR;
  if (state == STATE_ERROR) lastError = "Validierung fehlgeschlagen. Gelesen: " + readBack;
  cardReady = false;
  redirectRoot();
}

static void handleReset() {
  pendingContent = "";
  lastReadContent = "";
  lastUid = "";
  lastError = "";
  cardReady = false;
  state = STATE_INPUT;
  redirectRoot();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  SPI.begin(NFC_SCK_PIN, NFC_MISO_PIN, NFC_MOSI_PIN, NFC_SS_PIN);
  nfc.PCD_Init();

  uint64_t mac = ESP.getEfuseMac();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "%s%04X", AP_SSID_PREFIX, (uint16_t)(mac & 0xFFFF));

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_NETMASK);
  WiFi.softAP(ssid, AP_PASSWORD);
  dnsServer.start(53, "*", AP_IP);

  server.on("/", HTTP_GET, renderPage);
  server.on("/content", HTTP_POST, handleContent);
  server.on("/confirm", HTTP_POST, handleConfirm);
  server.on("/write", HTTP_POST, handleWrite);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(renderPage);
  server.begin();

  Serial.printf("[NFC-WRITER] AP %s ready at http://192.168.4.1\n", ssid);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (state == STATE_WAIT_CARD && !cardReady && pollCard()) {
    cardReady = true;
    state = STATE_READY_TO_WRITE;
    Serial.printf("[NFC-WRITER] Card ready: %s\n", lastUid.c_str());
  }
}
