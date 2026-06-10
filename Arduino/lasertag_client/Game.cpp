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
  if (g_phaseLastUpdate != 0 && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS) {
    if (g_phase == PHASE_IDLE) return PHASE_ACTIVE;
    return g_phase;
  }
  if (g_standalonePhase != PHASE_IDLE) return g_standalonePhase;
  return PHASE_ACTIVE;
}

bool isShootingAllowed() {
  return effectivePhase() == PHASE_ACTIVE;
}

static uint32_t resolveShooterColor(uint32_t shooterPlayerId) {
  uint8_t cmd = (uint8_t)((shooterPlayerId >> 8) & 0xFF);
  bool teamsFresh = g_teamCount > 0 && (millis() - g_teamsLastUpdate) < TEAMS_TIMEOUT_MS;
  if (teamsFresh) {
    uint32_t bit = (cmd >= 1 && cmd <= 32) ? (1u << (cmd - 1)) : 0u;
    for (uint8_t i = 0; i < g_teamCount; i++) {
      if (g_teams[i].memberBits & bit) return g_teams[i].color;
    }
  }
  if (cmd >= 1) return COLOR_POOL[(cmd - 1) % COLOR_POOL_SIZE];
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
      if (isPlayerDisabled()) { updateDisplay(); return true; }
      if (!isShootingAllowed()) { updateDisplay(); return true; }
      if (!identityIsAssigned()) { updateDisplay(); return true; }

      const uint32_t myPlayerId = getMyPlayerId();
      irsend.sendNEC(static_cast<uint64_t>(myPlayerId), 32);
      lastShotAt = millis();
      shotsFired++;
      Serial.printf("[SHOT] %s sent NEC 0x%lx, shot #%d\n",
                    identityMyName(), (unsigned long)myPlayerId, shotsFired);
      broadcastPlayerState();
      updateDisplay();
    }
  }
  return true;
}

static void resetHitCounters() {
  hitCount = 0;
  hitCountPrimary = 0;
  hitCountSecondary = 0;
  hitCountTertiary = 0;
}

static int teamIndexForPlayer(uint32_t playerId) {
  uint8_t cmd = (uint8_t)((playerId >> 8) & 0xFF);
  bool teamsFresh = g_teamCount > 0 && (millis() - g_teamsLastUpdate) < TEAMS_TIMEOUT_MS;
  if (!teamsFresh) return -1;
  uint32_t bit = (cmd >= 1 && cmd <= 32) ? (1u << (cmd - 1)) : 0u;
  if (!bit) return -1;
  for (uint8_t i = 0; i < g_teamCount; i++) {
    if (g_teams[i].memberBits & bit) return i;
  }
  return -1;
}

static bool isFriendlyFire(uint32_t shooterPlayerId) {
  if (strcmp(g_phaseMode, "team") != 0) return false;
  int shooterTeam = teamIndexForPlayer(shooterPlayerId);
  int myTeam = teamIndexForPlayer(getMyPlayerId());
  return shooterTeam >= 0 && myTeam >= 0 && shooterTeam == myTeam;
}

static bool handleIrReceiverInput(IRrecv &receiver,
                                  decode_results &decoded,
                                  const char *sensorName,
                                  int &sensorHitCount,
                                  int zonePoints) {
  if (!receiver.decode(&decoded)) return true;

  if (decoded.decode_type != NEC || decoded.bits != 32) {
    receiver.resume();
    return true;
  }

  const uint32_t shooterId = irsend.encodeNEC(decoded.address, decoded.command);
  const char *shooterName = findPlayerNameById(shooterId);
  char shooterNameCopy[12] = {};
  const bool withinSelfHitWindow = millis() - lastShotAt < SELF_HIT_IGNORE_MS;
  if (shooterName) {
    strncpy(shooterNameCopy, shooterName, sizeof(shooterNameCopy) - 1);
    shooterNameCopy[sizeof(shooterNameCopy) - 1] = '\0';
  } else {
    uint8_t cmd = (uint8_t)((shooterId >> 8) & 0xFF);
    snprintf(shooterNameCopy, sizeof(shooterNameCopy), "Cmd%03u", (unsigned)cmd);
  }

  if (decoded.address != IR_GAME_ADDRESS) {
  } else if (isPlayerDisabled()) {
  } else if (!isShootingAllowed()) {
  } else if (shooterId == getMyPlayerId() && withinSelfHitWindow) {
  } else if (shooterId != getMyPlayerId() && !isFriendlyFire(shooterId)) {
    hitCount++;
    sensorHitCount++;
    playerDisabledUntil = millis() + HIT_DISABLE_MS;
    uint32_t shooterColor = resolveShooterColor(shooterId);
    triggerHitBlink(shooterColor);
    Serial.printf("[HIT:%s] from %s (color #%06lx, sensor hits %d/%d/%d, +%d)\n",
                  sensorName,
                  shooterNameCopy[0] ? shooterNameCopy : "?",
                  (unsigned long)shooterColor,
                  hitCountPrimary,
                  hitCountSecondary,
                  hitCountTertiary,
                  zonePoints);

    if (awardPointsToPlayer(shooterId, shooterNameCopy, zonePoints)) {
      queueTableBroadcast();
    }
    broadcastHitEvent(shooterId, shooterNameCopy, zonePoints);
    broadcastPlayerState();
    updateDisplay();
  }

  receiver.resume();
  return true;
}

bool handleIrReceiver() {
  bool ok = true;
  ok = handleIrReceiverInput(irrecv, results, "ZONE1", hitCountPrimary, g_zonePoints[0]) && ok;
  if (HAS_ZONE2_RECEIVER) {
    ok = handleIrReceiverInput(irrecvSecondary,
                               resultsSecondary,
                               "ZONE2",
                               hitCountSecondary,
                               g_zonePoints[1]) && ok;
  }
  if (HAS_ZONE3_RECEIVER) {
    ok = handleIrReceiverInput(irrecvTertiary,
                               resultsTertiary,
                               "ZONE3",
                               hitCountTertiary,
                               g_zonePoints[2]) && ok;
  }
  return ok;
}

void resetRuntimeStatsForNewMatch() {
  shotsFired = 0;
  myPoints = 0;
  resetHitCounters();
}

void updateRespawnDisplayState(bool disabledNow, bool &wasPlayerDisabled) {
  if (disabledNow) {
    if (millis() - lastDisplayRefreshAt >= 150) updateDisplay();
  } else if (wasPlayerDisabled) {
    applyTeamColor();
    updateDisplay();
  }
  wasPlayerDisabled = disabledNow;
}

// === Standalone-State-Machine (v4 + v4.1 Master-Election) ================
// Master (= niedrigste MAC unter den bekannten Peers) treibt die State-
// Machine voran. Andere Clients syncen ihre Timer per MSG_STANDALONE.
//
// Wenn länger als STANDALONE_TIMEOUT_MS keine Master-Nachricht eingegangen
// ist, übernimmt der aktuelle Knoten (sofern er Master ist) selbst die
// Initiative — oder bleibt im aktuellen Zustand bis der Master wieder da
// ist (passive Knoten).
static unsigned long kStandaloneStateChangedAt = 0;

void standaloneStateTick(int peerCountIncludingSelf) {
  bool serverActive = (g_phaseLastUpdate != 0 && (millis() - g_phaseLastUpdate) < PHASE_TIMEOUT_MS);
  if (serverActive) {
    g_standalonePhase = PHASE_IDLE;
    return;
  }
  if (!identityIsAssigned()) return;

  unsigned long now = millis();
  bool iAmMaster = identityIsLobbyMaster();
  bool masterFresh = (g_standaloneLastUpdate != 0) &&
                     (now - g_standaloneLastUpdate < STANDALONE_TIMEOUT_MS);

  // Wenn ein Master existiert und nicht ich bin → passive: warten auf
  // MSG_STANDALONE, lokale Timer werden durch handleStandalone() gesetzt.
  // Wenn der lokal aktive Phasenstand abgelaufen ist, einfach Timer-Update
  // dem Master überlassen.
  if (!iAmMaster && masterFresh) {
    // Lokal nichts ausrichten — Master treibt voran
    return;
  }

  // Ich bin Master ODER kein frischer Master da → ich entscheide
  switch (g_standalonePhase) {
    case PHASE_IDLE:
      if (peerCountIncludingSelf >= 2 && iAmMaster) {
        g_standalonePhase = PHASE_LOBBY;
        g_standaloneLobbyEnds = now + STANDALONE_LOBBY_SECONDS * 1000UL;
        kStandaloneStateChangedAt = now;
        Serial.printf("[STANDALONE] Master starts LOBBY (%lus)\n",
                      (unsigned long)STANDALONE_LOBBY_SECONDS);
        memset(ranking, 0, sizeof(ranking));
        resetRuntimeStatsForNewMatch();
        updateDisplay();
      }
      break;
    case PHASE_LOBBY:
      if ((long)(now - g_standaloneLobbyEnds) >= 0 && iAmMaster) {
        g_standalonePhase = PHASE_ACTIVE;
        g_standaloneMatchEnds = now + STANDALONE_MATCH_SECONDS * 1000UL;
        kStandaloneStateChangedAt = now;
        Serial.printf("[STANDALONE] Master moves to ACTIVE (%lus)\n",
                      (unsigned long)STANDALONE_MATCH_SECONDS);
        updateDisplay();
      }
      break;
    case PHASE_ACTIVE:
      if ((long)(now - g_standaloneMatchEnds) >= 0 && iAmMaster) {
        g_standalonePhase = PHASE_DONE;
        kStandaloneStateChangedAt = now;
        Serial.println("[STANDALONE] Master moves to DONE");
        updateDisplay();
      }
      break;
    case PHASE_DONE:
      if (now - kStandaloneStateChangedAt > 15000UL && iAmMaster) {
        g_standalonePhase = PHASE_IDLE;
        kStandaloneStateChangedAt = now;
        updateDisplay();
      }
      break;
  }
}
