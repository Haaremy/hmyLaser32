# Arduino-Code — hmyLaser32

Modulares Arduino-Projekt für die beiden Rollen des Systems:

## `lasertag_client/`
Code für den **Player-ESP** (Pistole + Brustmodul). IR-Sender + IR-Empfänger
über NEC-Protokoll, OLED-Anzeige, RGB-Status-LED, ESP-NOW-Peer-to-Peer mit
allen anderen Clients zum Rank-Sync.

Setze pro Spieler eine eindeutige Player-ID in `Config.h`:

```cpp
constexpr char MY_NAME[] = "PLAYER_1";
constexpr uint8_t MY_IR_COMMAND = 0x01;   // 0x01..0xFE pro Gerät
```

## `lasertag_server/`
Code für den **Bridge-ESP** (Server). Keine IR-Hardware, nur ein ESP32
NodeMCU-32S an einer Powerbank. Aufgaben:

- ESP-NOW lauschen (Channel 1, gleicher wie Clients)
- WLAN-Verbindung via Captive Portal mit Scan + persistenter Speicherung
- Self-Register beim Web-Portal `laser32.haaremy.de`
- Match-Orchestrierung mit Lobby-Phase (z. B. 60 s für Verteilen + Verstecken)
- Hit-Forwarding: erkennt Punkt-Diffs in den ESP-NOW-Tables und sendet sie
  als Hit-Events an `POST /api/bridge/hit`

### Erste Inbetriebnahme

1. Code in Arduino IDE öffnen (oder PlatformIO):
   `Arduino/lasertag_server/lasertag_server.ino`
2. Board: **ESP32 Dev Module** (oder konkret NodeMCU-32S).
   Upload Speed 115 200, Flash Frequency 40 MHz, **Erase all Flash on upload**.
3. Bibliotheken installieren:
   - WiFi (Board-Manager)
   - ESPAsyncWebServer **nicht nötig** — wir nutzen den eingebauten `WebServer`
   - `Preferences` (Board-Manager)
   - `HTTPClient` (Board-Manager)
4. Hochladen, anschließen, Serial-Monitor öffnen.
5. Im Smartphone-WLAN nach `hmyLaser32-XXXX` suchen, verbinden.
6. Auf `http://192.168.4.1` werden Identität (Name + PIN), WLAN-Scan und
   Match-Settings angezeigt.
7. WLAN auswählen, Passwort eingeben, speichern → ESP rebootet, verbindet
   sich, registriert sich.
8. PIN merken — er ist für Settings-Edit und Match-Start nötig.

### Match-Workflow

| Phase | Was passiert |
|-------|--------------|
| **IDLE** | Server ist registriert, wartet auf Match-Trigger |
| **LOBBY** | `lobbySeconds` läuft (Default 60 s). Spieler verteilen sich. |
| **ACTIVE** | Server ruft `POST /api/bridge/match/start`; Match läuft. |
| **DONE** | Server schickt finale Statistik via `POST /api/bridge/match/end`. |

Trigger des Matches:
- Lokal: `POST http://<server-ip>/api/match/start` mit Form-Feld `pin=<PIN>`
- Lokale Web-UI: Button auf `http://<server-ip>/`
- Remote: Portal-UI auf `https://laser32.haaremy.de/esp/<server-id>` (PIN-Eingabe)

### NVS-Reset

Falls Name oder PIN vergessen: Im Captive-Portal `Reset Name+PIN` mit
aktuellem PIN bestätigen. Oder: ESP neu flashen mit aktiviertem
`Erase all Flash on upload`.
