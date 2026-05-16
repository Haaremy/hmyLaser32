#ifndef LASERTAG_SERVER_WIFI_H
#define LASERTAG_SERVER_WIFI_H

#include <Arduino.h>

// Schaltet das Gerät in AP+STA-Mode. Der AP bleibt PERMANENT aktiv;
// der STA-Connect ist optional und wird mit gespeicherten Credentials versucht.
// Liefert true wenn STA verbunden, false wenn nur AP läuft.
bool wifiBegin();

// Löscht WLAN-Credentials, ohne Reboot.
void wifiForgetCredentials();

// Periodischer Tick (DNS-Hijack + Reconnect-Watchdog).
void wifiLoop();

// Erneuter STA-Connect-Versuch (z.B. nach Settings-Update).
bool wifiTryReconnect();

// JSON-Array umliegender WLANs (synchroner Scan).
String wifiScanJson();

#endif
