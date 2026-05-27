# hmyLaser32 Firmware

Modular Arduino firmware for two ESP32 roles:

```text
Arduino/
|-- lasertag_client/   # player ESP: IR, OLED, WS2812B, button, optional NFC
`-- lasertag_server/   # server ESP: captive portal, ESP-NOW, HTTPS bridge
```

Both sketches target the Espressif ESP32 Arduino core and can be built with
Arduino IDE 2.x or PlatformIO.

## Hardware

### Player ESP

| Part | Notes |
|---|---|
| ESP32 NodeMCU-32S | main controller |
| KY-005 IR emitter | trigger output, NEC encoded |
| TSOP38238 IR receiver | primary hit zone |
| Second TSOP38238 | optional second hit zone, currently supported in code |
| SSD1306 128x64 OLED | status display |
| WS2812B strip | player/team color and hit feedback |
| Momentary button | trigger, default GPIO 32 |
| USB powerbank | 5 V supply |
| MFRC522 NFC reader | optional, enable with `HAS_NFC=1` |

### Server ESP

| Part | Notes |
|---|---|
| ESP32 NodeMCU-32S | AP, captive portal, ESP-NOW bridge |
| USB powerbank | mobile power |

No IR hardware is required for the server.

## Client Pins

Defined in `lasertag_client/Config.h`.

| Function | GPIO |
|---|---|
| IR send | 4 |
| IR receive zone 1 | 13 |
| IR receive zone 2 | 26 |
| Trigger button | 32 |
| OLED SDA | 21 |
| OLED SCL | 22 |
| WS2812B data | 27 |
| NFC SS | 5 |
| NFC RST | 33 |
| NFC SPI | MOSI 23, MISO 19, SCK 18 |

For long WS2812B strips, feed 5 V directly from the power source and share GND
with the ESP32. A 74HCT245 level shifter or equivalent is recommended.

## Required Libraries

Install via Arduino IDE Library Manager:

| Library | Used by | Purpose |
|---|---|---|
| `IRremoteESP8266` | client | send/receive NEC IR |
| `U8g2` | client | OLED display |
| `FastLED` | client | WS2812B strip |
| `MFRC522` | client | NFC reader when enabled |

Built into the ESP32 core:

- `WiFi`
- `esp_now`
- `esp_wifi`
- `HTTPClient`
- `WiFiClientSecure`
- `Preferences`
- `ESPmDNS`
- `DNSServer`
- `WebServer`

## Quick Config

Client quick settings are at the top of `lasertag_client/Config.h`.

```cpp
#define LED_STRIP_COUNT 80
#define HAS_NFC 0
#define LED_BRIGHTNESS 80
```

Identity is negotiated automatically. A client starts with an auto-name and can
later override it by scanning an NFC card with:

```text
username|uuid-token
```

## Client Firmware

Important modules:

| File | Responsibility |
|---|---|
| `lasertag_client.ino` | setup/loop, ESP-NOW init, periodic timers |
| `Config.h` | pins, timing, message IDs |
| `Types.h` | shared ESP-NOW wire structs |
| `Game.cpp` | trigger, IR receive, scoring, respawn, friendly-fire check |
| `Network.cpp` | ESP-NOW receive/send, player state, config, hit events |
| `Ranking.cpp` | ranking table anti-entropy |
| `Identity.cpp` | auto name, command ID, color, lobby master election |
| `Nfc.cpp` | optional card scan and username/token sync |
| `Display.cpp` | OLED screens |
| `Led.cpp` | WS2812B color and hit blink |

### Runtime Stats

The client tracks:

- `shotsFired`: trigger pulls sent by this client
- `hitCount`: RX hits received by this client
- `myPoints`: current points for this client
- ranking entries for known players
- current server phase/mode/hit-points
- assigned color/team from the server

`shotsFired`, `hitCount`, `myPoints`, name, color, team, and NFC token are sent
to the server with `MSG_PLAYER_STATE`.

### Scoring

- Shots are allowed only during `PHASE_ACTIVE`.
- A valid hit awards `g_hitPoints` to the shooter.
- Default hit points are `10`.
- Team-mode friendly fire is ignored.
- The receiving client broadcasts `MSG_HIT_EVENT` with shooter and target data.

## Server Firmware

Important modules:

| File | Responsibility |
|---|---|
| `lasertag_server.ino` | setup/loop, global state |
| `Config.h` | portal URL, AP prefix, match defaults |
| `Types.h` | shared wire structs, snapshots, settings |
| `Portal.cpp` | local captive portal UI and endpoints |
| `EspNow.cpp` | ESP-NOW receive/send, snapshots, config broadcast |
| `Match.cpp` | IDLE -> LOBBY -> ACTIVE -> DONE state machine |
| `Bridge.cpp` | HTTPS calls to Next.js portal |
| `WiFiSetup.cpp` | permanent AP + STA + mDNS |
| `Identity.cpp` | server name and PIN generation |
| `Storage.cpp` | NVS wrapper |

### Local Portal

The server opens a permanent AP:

```text
hmyLaser32-XXXX
http://192.168.4.1
```

The portal has two tabs.

`Spiel`:

- start waiting room
- set distribution time, match duration, hit points
- select mode: `Alle gegen Alle` or `Team-Modus`
- edit known players: color, device ID, name, team
- sync players with `Aktualisieren`
- start match from lobby
- show live statistics: rank, player, shots, RX hits, points

`Einstellungen`:

- server name/PIN
- AP and STA data
- portal registration state
- WLAN scan/save/forget

There is no early match abort route in the UI.

### Match Flow

```text
IDLE
  -> Warteraum starten
LOBBY
  -> Match starten or lobby timer expires
ACTIVE
  -> match timer expires
DONE
```

During `LOBBY` and `ACTIVE`, the server broadcasts phase/config/team updates.
When the active timer expires, the server posts final stats to the web portal.

## ESP-NOW Wire Format

`Types.h` on both client and server must stay binary-compatible.

```cpp
struct __attribute__((packed)) RankingEntry {
  uint32_t playerId;
  char     player[12];
  int16_t  points;
  uint32_t lastUpdate;
};

struct __attribute__((packed)) Message {
  char        senderName[12];
  uint8_t     msgType;
  uint8_t     playerCount;
  RankingEntry entries[MAX_PLAYERS];
};
```

Do not add fields to these structs unless every device is migrated together.
Use a new `MSG_*` type and encode payloads into `entries[]`.

## Message Types

| Type | Sender | Payload |
|---|---|---|
| `MSG_DISCOVERY` | client | presence and optional self entry |
| `MSG_TABLE` | client | legacy full ranking table |
| `MSG_PHASE` | server | phase, seconds left, mode, hit points |
| `MSG_TEAMS` | server | team names/colors/member bitmasks |
| `MSG_NFC` | client | player ID, username, UUID chunks |
| `MSG_STANDALONE` | client | no-server phase consensus |
| `MSG_PLAYER_STATE` | client | points, shots, RX hits, color, team, token |
| `MSG_PLAYER_CONFIG` | server | assigned name/color/team |
| `MSG_HIT_EVENT` | client | shooter, target, points, receiver stats |

## Bridge/Web Sync

The server forwards:

- registration and heartbeat
- match start/end
- live hit events
- known player snapshots with `points`, `shots`, `rxHits`, `team`, `color`,
  and optional `nfcToken`
- pulled settings: mode, lobby seconds, match seconds, hit points, teams,
  and start-request flag

The ESP server stores no long-term player data. Persistent player history lives
in the web portal database.

## Upload Settings

Recommended Arduino IDE settings:

| Setting | Value |
|---|---|
| Board | ESP32 Dev Module or NodeMCU-32S |
| Upload speed | 115200 |
| Flash frequency | 40 MHz |
| Flash mode | QIO |
| Partition scheme | default 4 MB |
| Erase flash | Yes during development |

## Debugging

Serial baud: `115200`.

Useful prefixes:

| Prefix | Meaning |
|---|---|
| `[IDENTITY]` | client identity negotiation |
| `[CONFIG]` | client received server color/team config |
| `[SHOT]` | trigger fired |
| `[HIT:*]` | IR hit received on a sensor zone |
| `[STANDALONE]` | no-server lobby state |
| `[NFC]` | card scan and token handling |
| `[matchLoop]` | server match phase transitions |
| `[bridge...]` | HTTPS bridge calls |

If ESP-NOW is silent, first check WiFi channel alignment. The ESP32 AP and STA
must share a channel, and clients need to use the same channel as the server.

## Extending

- Keep `Message` and `RankingEntry` stable.
- Add new behavior with new `MSG_*` constants.
- Document slot semantics next to the message type.
- Keep `loop()` non-blocking; web requests may block briefly, periodic tasks
  should not.
- Add bridge endpoints in both `src/app/api/...` and
  `lasertag_server/Bridge.cpp`.
