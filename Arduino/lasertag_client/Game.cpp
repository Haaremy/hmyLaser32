#include "Game.h"
#include <IRutils.h>
#include <cstring>
#include "Config.h"
#include "Display.h"
#include "Globals.h"
#include "Identity.h"
#include "Led.h"
#include "LasertagNetwork.h"
#include "Ranking.h"

uint32_t getMyPlayerId() {
  return irsend.encodeNEC(IR_GAME_ADDRESS, identityMyCommand());
}

bool isPlayerDisabled() {
  return static_cast<long>(playerDisabledUntil - millis()) > 0;
}

unsigned long getDisableTimeLeftMs() {
  if (!isPlayerDisabled()) return 0;
  return playerDisabledUntil - millis();
}

static uint8_t effectivePhase() {
  // Server-aware phase (vom Server broadcastet)
  if (g_phaseLastUpdate != 0 && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS) {
    return g_phase;
  }
  // Server-Fallback: standalone-Phase (Konsens via Discovery)
  return g_standalonePhase;
}

bool isShootingAllowed() {
  return effectivePhase() == PHASE_ACTIVE;
}

// Lookup der Schützen-Farbe aus den Teams (Team-Modus) oder Identitäts-
// Farbe (FFA, abgeleitet von Player-Index = command - 1).
static uint32_t resolveShooterColor(uint32_t shooterPlayerId) {
  // command-Byte aus NEC playerId (bits 8..15)
  uint8_t cmd = (uint8_t)((shooterPlayerId >> 8) & 0xFF);
  // 1) Team-Modus mit gültiger Teams-Liste
  bool teamsFresh = g_teamCount > 0 && (millis() - g_teamsLastUpdate) < TEAMS_TIMEOUT_MS;
  if (teamsFresh) {
    uint32_t bit = (cmd >= 1 && cmd <= 32) ? (1u << (cmd - 1)) : 0u;
    for (uint8_t i = 0; i < g_teamCount; i++) {
      if (g_teams[i].memberBits & bit) return g_teams[i].color;
    }
  }
  // 2) FFA: COLOR_POOL[(cmd - 1) % POOL_SIZE]
  if (cmd >= 1) {
    return COLOR_POOL[(cmd - 1) % COLOR_POOL_SIZE];
  }
  return 0xFFFFFF;
}

bool handleTrigger() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastRawButtonReading) {
    lastDebounce = millis();
    lastRawButtonReading = reading;
  }

  if ((millis() - lastDebounce) > DEBOUNCE_MS && reading != lastButtonState) {
    lastButtonState = reading;

    if (reading == LOW) {
      if (isPlayerDisabled()) { updateDisplay(); return false; }
      if (!isShootingAllowed()) { updateDisplay(); return false; }
      if (!identityIsAssigned()) { updateDisplay(); return false; }   // Wait-Mode

      const uint32_t myPlayerId = getMyPlayerId();
      irsend.sendNEC(static_cast<uint64_t>(myPlayerId), 32);
      lastShotAt = millis();
      shotsFired++;
      Serial.printf("[SHOT] %s sent NEC 0x%lx, shot #%d\n", identityMyName(), (unsigned long)myPlayerId, shotsFired);
      updateDisplay();
    }
  }
  return true;
}

bool handleIrReceiver() {
  if (!irrecv.decode(&results)) return true;

  if (results.decode_type != NEC || results.bits != 32) {
    irrecv.resume();
    return false;
  }

  const uint32_t shooterId = irsend.encodeNEC(results.address, results.command);
  const char *shooterName = findPlayerNameById(shooterId);
  char shooterNameCopy[12] = {};
  const bool withinSelfHitWindow = millis() - lastShotAt < SELF_HIT_IGNORE_MS;
  if (shooterName) {
    strncpy(shooterNameCopy, shooterName, sizeof(shooterNameCopy) - 1);
    shooterNameCopy[sizeof(shooterNameCopy) - 1] = '\0';
  }

  if (results.address != IR_GAME_ADDRESS) {
    // Fremdes Protokoll
  } else if (isPlayerDisabled()) {
    // Schon down
  } else if (!isShootingAllowed()) {
    // Falsche Phase
  } else if (shooterId == getMyPlayerId() && withinSelfHitWindow) {
    // Self-hit Schutzfenster
  } else if (shooterId != getMyPlayerId()) {
    // === Treffer registriert ===
    // v4: Friendly Fire ist erlaubt — keine Team-Check-Logik mehr.
    hitCount++;
    playerDisabledUntil = millis() + HIT_DISABLE_MS;
    uint32_t shooterColor = resolveShooterColor(shooterId);
    triggerHitBlink(shooterColor);
    Serial.printf("[HIT] from %s (color #%06lx), respawn %lu s\n",
                  shooterNameCopy[0] ? shooterNameCopy : "?",
                  (unsigned long)shooterColor,
                  HIT_DISABLE_MS / 1000UL);

    if (shooterName != nullptr && awardPointsToPlayer(shooterId, shooterNameCopy, 10)) {
      queueTableBroadcast();
    }
    updateDisplay();
  }

  irrecv.resume();
  return true;
}

void updateRespawnDisplayState(bool disabledNow, bool &wasPlayerDisabled) {
  if (disabledNow) {
    // LED wird durch ledTickHitBlink() weitergeführt
    if (millis() - lastDisplayRefreshAt >= 150) updateDisplay();
  } else if (wasPlayerDisabled) {
    // Respawn fertig → zurück auf Team-/Identitäts-Farbe
    applyTeamColor();
    updateDisplay();
  }
  wasPlayerDisabled = disabledNow;
}

// v4: Stand-Alone-Lobby/Match-State-Machine.
// Wird in loop() aufgerufen. Sobald der Client ≥2 Peers kennt (sich selbst +
// 1 anderen) UND kein Server aktiv ist, startet die Lobby automatisch nach
// einer kurzen Verzögerung. Ablauf:
//   IDLE  → (peer count ≥ 2)            → LOBBY
//   LOBBY → (STANDALONE_LOBBY_SECONDS)  → ACTIVE
//   ACTIVE→ (STANDALONE_MATCH_SECONDS)  → DONE
//   DONE  → (PHASE_TIMEOUT_MS gelagert) → IDLE
static unsigned long kStandaloneStateChangedAt = 0;

void standaloneStateTick(int peerCountIncludingSelf) {
  bool serverActive = (g_phaseLastUpdate != 0 && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS);
  if (serverActive) {
    g_standalonePhase = PHASE_IDLE;
    return;
  }
  unsigned long now = millis();
  switch (g_standalonePhase) {
    case PHASE_IDLE:
      if (peerCountIncludingSelf >= 2 && identityIsAssigned()) {
        g_standalonePhase = PHASE_LOBBY;
        g_standaloneLobbyEnds = now + STANDALONE_LOBBY_SECONDS * 1000UL;
        kStandaloneStateChangedAt = now;
        Serial.printf("[STANDALONE] LOBBY (%lus)\n", (unsigned long)STANDALONE_LOBBY_SECONDS);
        memset(ranking, 0, sizeof(ranking));
        hitCount = 0; shotsFired = 0; myPoints = 0;
        updateDisplay();
      }
      break;
    case PHASE_LOBBY:
      if ((long)(now - g_standaloneLobbyEnds) >= 0) {
        g_standalonePhase = PHASE_ACTIVE;
        g_standaloneMatchEnds = now + STANDALONE_MATCH_SECONDS * 1000UL;
        kStandaloneStateChangedAt = now;
        Serial.printf("[STANDALONE] ACTIVE (%lus)\n", (unsigned long)STANDALONE_MATCH_SECONDS);
        updateDisplay();
      }
      break;
    case PHASE_ACTIVE:
      if ((long)(now - g_standaloneMatchEnds) >= 0) {
        g_standalonePhase = PHASE_DONE;
        kStandaloneStateChangedAt = now;
        Serial.println("[STANDALONE] DONE");
        updateDisplay();
      }
      break;
    case PHASE_DONE:
      if (now - kStandaloneStateChangedAt > 15000UL) {
        g_standalonePhase = PHASE_IDLE;
        kStandaloneStateChangedAt = now;
        updateDisplay();
      }
      break;
  }
}
