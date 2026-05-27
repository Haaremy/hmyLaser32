#include "Nfc.h"
#include "Config.h"
#include "Game.h"
#include "Globals.h"
#include "Identity.h"
#include "LasertagNetwork.h"

#if HAS_NFC
  #include <SPI.h>
  #include <MFRC522.h>
  static MFRC522 mfrc522(NFC_SS_PIN, NFC_RST_PIN);
#endif

static char kLastUsername[33] = "";
static char kLastToken[40]    = "";
static bool kBound = false;

void nfcBegin() {
#if HAS_NFC
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("[NFC] MFRC522 ready");
#else
  Serial.println("[NFC] disabled (HAS_NFC=0)");
#endif
}

// Sehr simple Karten-Parser-Heuristik: lese die NDEF-Daten bzw. Block 4 als
// ASCII; Format "username|token". Wenn keine Library vorhanden, wird hier ein
// Stub eingesetzt — der echte Reader-Code muss je nach Karte (Mifare Classic
// vs. NTAG vs. PN532 vs. MFRC522) angepasst werden.
static bool nfcReadCard(char *out, size_t maxLen) {
#if HAS_NFC
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;

  // Mifare Classic: Block 4 lesen (Default-Key A FFFFFFFFFFFF)
  MFRC522::MIFARE_Key key;
  for (uint8_t i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  byte buffer[18];
  byte size = sizeof(buffer);
  uint8_t status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return false;
  }
  status = mfrc522.MIFARE_Read(4, buffer, &size);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  if (status != MFRC522::STATUS_OK) return false;

  size_t copyLen = (size < maxLen - 1) ? size : (maxLen - 1);
  memcpy(out, buffer, copyLen);
  out[copyLen] = '\0';
  return true;
#else
  (void)out; (void)maxLen;
  return false;
#endif
}

void nfcLoop() {
#if HAS_NFC
  static unsigned long lastScan = 0;
  if (millis() - lastScan < 500) return;
  lastScan = millis();

  char raw[64] = {};
  if (!nfcReadCard(raw, sizeof(raw))) return;

  // Parse "<username>|<token>"
  char *sep = strchr(raw, '|');
  if (!sep) {
    Serial.println("[NFC] no separator in card data");
    return;
  }
  *sep = '\0';
  strlcpy(kLastUsername, raw, sizeof(kLastUsername));
  strlcpy(kLastToken,    sep + 1, sizeof(kLastToken));
  identitySetName(kLastUsername);

  Serial.printf("[NFC] scanned username=%s token=%s\n", kLastUsername, kLastToken);

  // MSG_NFC an Server-ESP broadcasten.
  Message m = {};
  strlcpy(m.senderName, kLastUsername, sizeof(m.senderName));
  m.msgType = MSG_NFC;
  m.playerCount = 4;
  m.entries[0].playerId = getMyPlayerId();
  strlcpy(m.entries[0].player, kLastUsername, sizeof(m.entries[0].player));
  for (int i = 0; i < 3; i++) {
    strncpy(m.entries[i + 1].player, kLastToken + (i * 11), sizeof(m.entries[i + 1].player) - 1);
  }
  const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcast, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
  broadcastPlayerState();

  kBound = true;
#endif
}

const char *nfcLastUsername() { return kLastUsername; }
const char *nfcLastToken()    { return kLastToken; }
bool        nfcIsBound()      { return kBound; }
