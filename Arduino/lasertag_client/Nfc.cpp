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

// Mifare Classic: ASCII-Daten aus mehreren Datenbloecken lesen.
// Format: "username|token".
static bool nfcReadCard(char *out, size_t maxLen) {
#if HAS_NFC
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;

  MFRC522::MIFARE_Key key;
  for (uint8_t i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  size_t pos = 0;
  bool ok = true;
  for (size_t i = 0; i < NFC_DATA_BLOCK_COUNT && pos < maxLen - 1; i++) {
    byte block = NFC_DATA_BLOCKS[i];
    byte buffer[18] = {};
    byte size = sizeof(buffer);
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      ok = false;
      break;
    }
    status = mfrc522.MIFARE_Read(block, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      ok = false;
      break;
    }
    for (byte j = 0; j < 16 && pos < maxLen - 1; j++) {
      if (buffer[j] == 0) {
        out[pos] = '\0';
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return pos > 0;
      }
      out[pos++] = (char)buffer[j];
    }
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  out[pos] = '\0';
  return ok && pos > 0;
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

  char raw[96] = {};
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
  m.playerCount = 5;
  m.entries[0].playerId = getMyPlayerId();
  strlcpy(m.entries[0].player, kLastUsername, sizeof(m.entries[0].player));
  for (int i = 0; i < 4; i++) {
    strncpy(m.entries[i + 1].player, kLastToken + (i * 11), sizeof(m.entries[i + 1].player) - 1);
    m.entries[i + 1].player[sizeof(m.entries[i + 1].player) - 1] = '\0';
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
