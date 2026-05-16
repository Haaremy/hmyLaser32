#ifndef LASERTAG_SERVER_WIFI_H
#define LASERTAG_SERVER_WIFI_H

#include <Arduino.h>

// Versucht, mit den in NVS gespeicherten WLAN-Credentials zu verbinden.
// Liefert true bei erfolgreichem STA-Connect.
bool wifiTryStation();

// Startet den Captive-Portal-AP-Mode (offen) inkl. DNS-Hijack.
void wifiStartCaptive();

// Stoppt AP, wechselt in STA mit gespeicherten Creds.
bool wifiSwitchToStationFromStorage();

// Periodischer Tick — DNS + WiFi-Watchdog. In loop() aufrufen.
void wifiLoop();

// Scan: schreibt JSON-Array ("ssid", "rssi", "secure") nach `out`.
// `out` muss vom Caller reserviert sein.
String wifiScanJson();

#endif
