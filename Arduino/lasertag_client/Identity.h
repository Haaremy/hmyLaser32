#ifndef LASERTAG_CLIENT_IDENTITY_H
#define LASERTAG_CLIENT_IDENTITY_H

#include <Arduino.h>
#include "Config.h"

// ============================================================================
//   Auto-Identität-Aushandlung (v4)
//   Bei Boot generiert der Client eine "vorläufige" Identität aus den letzten
//   beiden MAC-Bytes. Während der Wartephase werden Peers gesammelt; nach
//   IDENTITY_WAIT_MS ohne neue Peers wird sortiert (kleinste MAC → A-Spieler)
//   und jeder bekommt einen Namen + Farbe aus den festen Listen.
//
//   Falls ein ESP-Server aktiv ist (MSG_PHASE empfangen), respektiert die
//   Auto-Vergabe das. Der Server kann Identitäten via MSG_TEAMS überschreiben.
// ============================================================================

constexpr int NAME_POOL_SIZE = 30;
constexpr int COLOR_POOL_SIZE = 10;
constexpr unsigned long IDENTITY_WAIT_MS = 3000;     // 3 s ohne neue Peers → assign
constexpr unsigned long IDENTITY_RECHECK_MS = 8000;  // periodisch neue Peers berücksichtigen

extern const char *NAME_POOL[NAME_POOL_SIZE];
extern const uint32_t COLOR_POOL[COLOR_POOL_SIZE];

void identityInit();
void identityLoop();

// Liefert den eigenen Anzeigenamen + Farbe. Wenn noch nicht ausgehandelt,
// gibt es einen "Searching..."-Platzhalter zurück.
const char *identityMyName();
uint32_t    identityMyColor();
uint8_t     identityMyCommand();      // 1..NAME_POOL_SIZE (NEC-Command)
bool        identityIsAssigned();     // false in der Wait-Phase

// Wird vom Network-Modul bei Discovery aufgerufen, um die Liste der
// aktuellen Peers zu pflegen. mac = 6 byte MAC, isServer markiert das
// Discovery als von einem Server-ESP stammend.
void identityPeerSeen(const uint8_t *mac, bool isServer);

#endif
