#include "Game.h"
#include <IRutils.h>
#include <cstring>
#include "Config.h"
#include "Display.h"
#include "Globals.h"
#include "Led.h"
#include "LasertagNetwork.h"
#include "Ranking.h"

uint32_t getMyPlayerId() {
  return irsend.encodeNEC(IR_GAME_ADDRESS, MY_IR_COMMAND);
}

bool isPlayerDisabled() {
  return static_cast<long>(playerDisabledUntil - millis()) > 0;
}

unsigned long getDisableTimeLeftMs() {
  if (!isPlayerDisabled()) {
    return 0;
  }
  return playerDisabledUntil - millis();
}

bool handleTrigger() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastRawButtonReading) {
    Serial.print("[BUTTON] Raw state: ");
    Serial.println(reading == LOW ? "LOW (pressed)" : "HIGH (released)");
    lastDebounce = millis();
    lastRawButtonReading = reading;
  }

  if ((millis() - lastDebounce) > DEBOUNCE_MS && reading != lastButtonState) {
    lastButtonState = reading;

    if (reading == LOW) {
      if (isPlayerDisabled()) {
        Serial.print("[SHOT] Ignored, player disabled for ");
        Serial.print(getDisableTimeLeftMs() / 1000UL + 1);
        Serial.println(" more s");
        updateDisplay();
        return false;
      }

      Serial.println("[SHOT] Trigger pressed, sending IR shot");
      const uint32_t myPlayerId = getMyPlayerId();
      irsend.sendNEC(static_cast<uint64_t>(myPlayerId), 32);
      lastShotAt = millis();
      shotsFired++;
      Serial.print("[SHOT] IR code sent: 0x");
      Serial.println(myPlayerId, HEX);
      Serial.print(">> Schuss #");
      Serial.println(shotsFired);
      updateDisplay();
    } else {
      Serial.println("[BUTTON] Trigger released");
    }
  }

  return true;
}

bool handleIrReceiver() {
  if (!irrecv.decode(&results)) {
    return true;
  }

  if (results.decode_type != NEC || results.bits != 32) {
    Serial.println("[IR] Ignored non-NEC or invalid-length frame");
    irrecv.resume();
    return false;
  }

  const uint32_t shooterId = irsend.encodeNEC(results.address, results.command);
  Serial.print("[IR] Received NEC value: 0x");
  Serial.println(shooterId, HEX);

  const char *shooterName = findPlayerNameById(shooterId);
  char shooterNameCopy[12] = {};
  const bool withinSelfHitWindow = millis() - lastShotAt < SELF_HIT_IGNORE_MS;

  if (shooterName != nullptr) {
    strncpy(shooterNameCopy, shooterName, sizeof(shooterNameCopy) - 1);
    shooterNameCopy[sizeof(shooterNameCopy) - 1] = '\0';
  }

  if (shooterName != nullptr) {
    Serial.print("[IR] Hit by ");
    Serial.println(shooterNameCopy);
  } else {
    Serial.println("[IR] Unknown shooter ID");
  }

  if (results.address != IR_GAME_ADDRESS) {
    Serial.print("[IR] Ignored foreign NEC address: 0x");
    Serial.println(results.address, HEX);
  } else if (isPlayerDisabled()) {
    Serial.println("[IR] Ignored hit, player currently disabled");
  } else if (shooterId == getMyPlayerId() && withinSelfHitWindow) {
    Serial.println("[IR] Ignored own shot during self-hit protection window");
  } else if (shooterId != getMyPlayerId()) {
    hitCount++;
    playerDisabledUntil = millis() + HIT_DISABLE_MS;
    setLedHitState();
    Serial.print("<< Treffer #");
    Serial.println(hitCount);
    Serial.print("[GAME] Player disabled for ");
    Serial.print(HIT_DISABLE_MS / 1000UL);
    Serial.println(" s");

    if (shooterName != nullptr && awardPointsToPlayer(shooterId, shooterNameCopy, 10)) {
      Serial.print("[IR] Awarded 10 points to ");
      Serial.println(shooterNameCopy);
      queueTableBroadcast();
    }

    updateDisplay();
  } else {
    Serial.println("[IR] Ignored own player ID");
  }

  irrecv.resume();
  return true;
}

void updateRespawnDisplayState(bool disabledNow, bool &wasPlayerDisabled) {
  if (disabledNow) {
    if (millis() - lastDisplayRefreshAt >= 150) {
      updateDisplay();
    }
  } else if (wasPlayerDisabled) {
    setLedNormalState();
    updateDisplay();
  }

  wasPlayerDisabled = disabledNow;
}
