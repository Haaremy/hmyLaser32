#ifndef LASERTAG_NFC_H
#define LASERTAG_NFC_H

#include <Arduino.h>

// ============================================================================
//   NFC-Account-Binding (v4)
//   Karten-Format: "<username>|<token>"  (Pipe-getrennt)
//     username : 3..32 ASCII-Zeichen [a-zA-Z0-9_.-]
//     token    : UUID (36 Zeichen mit Bindestrichen)
//
//   Beim Scan sendet der Client den Token via MSG_NFC an den Server-ESP.
//   Der Server-ESP verifiziert über /api/bridge/player/[token] und überträgt
//   den Anzeigenamen zurück. Bis dahin bleibt der Auto-Name aktiv.
//
//   Compile-Flag HAS_NFC in Config.h aktiviert die Hardware-Integration
//   (MFRC522 via SPI). Ohne Hardware: Stub.
// ============================================================================

void nfcBegin();
void nfcLoop();

// Read-only: was wurde zuletzt eingelesen + Status der Auflösung
const char *nfcLastUsername();      // "" wenn nichts gescannt
bool        nfcIsBound();           // true wenn Account erfolgreich gemappt

#endif
