#ifndef LASERTAG_SERVER_PORTAL_H
#define LASERTAG_SERVER_PORTAL_H

#include <Arduino.h>

// Lokaler Webserver: aktiv sowohl im AP- als auch im STA-Mode.
// Routen:
//   GET  /                   - HTML-Portal mit Tabs Spiel/Einstellungen
//   GET  /api/scan           - JSON-Array umliegender WLANs
//   POST /api/wifi           - speichert SSID+PSK in NVS, triggert Reboot
//   GET  /api/settings       - JSON: Identitaet, Match-Settings, Spieler
//   POST /api/settings       - speichert Match-Settings
//   POST /api/players        - speichert Farben/Teams und synchronisiert Clients
//   POST /api/match/start    - startet den Warteraum
//   POST /api/match/activate - beendet den Warteraum und startet das Match
//   GET  /api/status         - JSON: WLAN-State, Match-Phase, Live-Stats
//
// Das Match kann absichtlich nicht vorzeitig beendet werden.
void portalBegin();
void portalLoop();

#endif
