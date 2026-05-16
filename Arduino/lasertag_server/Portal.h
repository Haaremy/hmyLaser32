#ifndef LASERTAG_SERVER_PORTAL_H
#define LASERTAG_SERVER_PORTAL_H

#include <Arduino.h>

// Lokaler Webserver: aktiv sowohl im AP- als auch im STA-Mode.
// Routen:
//   GET  /                  — HTML-Konfig-Seite (WLAN-Scan + Match-Settings)
//   GET  /api/scan          — JSON-Array umliegender WLANs
//   POST /api/wifi          — speichert SSID+PSK in NVS, triggert Reboot
//   GET  /api/settings      — JSON: Identität + Match-Settings
//   POST /api/settings      — speichert Match-Settings (mode/lobby/match)
//   POST /api/match/start   — startet Match (akzeptiert optional ?pin=)
//   POST /api/identity/reset — generiert neue Identität (PIN-Gated)
//   GET  /api/status        — JSON: WLAN-State, Match-Phase, Ranking
//
// Im AP-Mode ist alles offen (Erstkonfig). Im STA-Mode sind schreibende
// Routen mit dem PIN abgesichert (Bearer oder Form-Feld).
void portalBegin();
void portalLoop();

#endif
