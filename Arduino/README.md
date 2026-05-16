# hmyLaser32 — Firmware

Modular Arduino code for two ESP32 roles. Both sketches are designed to
compile out-of-the-box with Arduino IDE 2.x or PlatformIO using the
**Espressif ESP32 Arduino core (board package: `esp32 by Espressif Systems`)**.

```
Arduino/
├── lasertag_client/   ← player ESP (IR + OLED + RGB-LED + button)
└── lasertag_server/   ← bridge ESP (Captive Portal + ESP-NOW + HTTPS)
```

## Table of contents

1. [Hardware bill of materials](#hardware-bill-of-materials)
2. [Pin assignment](#pin-assignment)
3. [Required libraries](#required-libraries)
4. [Upload settings](#upload-settings)
5. [Client (`lasertag_client/`)](#client-lasertag_client)
6. [Server (`lasertag_server/`)](#server-lasertag_server)
7. [ESP-NOW wire format](#esp-now-wire-format)
8. [Match state machine](#match-state-machine)
9. [Debugging & serial logs](#debugging--serial-logs)
10. [Extending the firmware](#extending-the-firmware)

---

## Hardware bill of materials

### Per player (`lasertag_client`)

| Part | Notes |
|---|---|
| ESP32 NodeMCU-32S | Dual-core 240 MHz, USB-programmable |
| TSOP38238 | IR receiver, 38 kHz, up to ~45 m range |
| KY-005 | IR emitter at 940 nm, drives directly from a GPIO |
| SSD1306 OLED 0.96″ I²C | 128 × 64 px status display |
| Momentary push button | Trigger, pull-up via internal resistor on GPIO 32 (default v4.2). Can be moved back to GPIO 19 if no NFC is used. |
| **WS2812B LED strip (30 LEDs)** | Status strip on the chest. One data line on GPIO 26, 5 V from VIN, GND. Default v4.2. |
| 74HCT245 level-shifter (recommended) | 3.3 V → 5 V on the WS2812B data line for strips > 10 LEDs. Alternative: 470 Ω resistor in series. |
| 1000 µF / 6.3 V capacitor (optional) | Between +5 V and GND at the strip start, against current spikes. |
| USB powerbank (5 V) | Power supply, ~6 h with a small cell. ESP draws ~200 mA, WS2812B adds up to ~50 mA per LED at full white. |
| Hookup wire + solder | Final assembly on perfboard |
| *(optional)* MFRC522 NFC reader | Account binding via NFC card. SPI on VSPI pins 18 / 19 / 23 / 5, RST on 33. |
| *(legacy)* 5 mm RGB LED + 3 × 330 Ω | Pre-v4.2: single RGB LED on GPIO 25 / 26 / 27. Use `#define USE_WS2812 0` to keep this mode. |

### Server (`lasertag_server`)

| Part | Notes |
|---|---|
| ESP32 NodeMCU-32S | Same MCU as the clients |
| USB powerbank | Power |
| *(optional)* OLED + button + RGB LED | Headless by default; wire identical to client for status |

> The current V1 setup has **no enclosure**. Components sit on perfboard
> with visible wires; the chest module is strapped to a vest, the pistol
> holds the IR LED + trigger + OLED.

---

## Pin assignment (v4.2 default — WS2812B strip + auto-identity)

```
ESP32 NodeMCU-32S
                 ┌───────────────────────────┐
                 │                           │
       3V3 ──────┤ 3.3 V                     │ → OLED VCC, TSOP VCC, KY-005 VCC
       VIN ──────┤ 5 V passthrough           │ → WS2812B +5 V (from USB powerbank)
       GND ──────┤ GND                       │ → all components common GND
                 │                           │
   IR-Sender ──◄─┤ GPIO  4  (IR_SEND_PIN)    │
   IR-Recv  ──►──┤ GPIO 14  (IR_RECV_PIN)    │
   Button ──►────┤ GPIO 32  (BUTTON_PIN)     │ INPUT_PULLUP
                 │                           │
   OLED SDA ─────┤ GPIO 21  (I²C SDA)        │
   OLED SCL ─────┤ GPIO 22  (I²C SCL)        │
                 │                           │
   LED Strip DIN ◄┤ GPIO 26 (LED_DATA_PIN)   │ → via 470 Ω or 74HCT245 level-shifter
                 │                           │
   (free for     │ GPIO 19 — if no NFC, you  │
    optional NFC)│   may put BUTTON_PIN here │
                 │                           │
                 └───────────────────────────┘
```

Notes:
- **5 V supply for the strip**: take it from the `VIN` (`5V`) pin, which
  is the USB-input passthrough. A typical NodeMCU-32S can pass ~500 mA;
  for a 30-LED strip at low brightness (the firmware caps at 80/255)
  total draw stays well under the powerbank limit.
- **Level shifter**: WS2812B expects ~5 V logic on the data line; the
  ESP32 outputs 3.3 V. For 10+ LEDs add a 74HCT245 or use the cheap
  "sacrifice the first LED as level shifter" trick (470 Ω resistor in
  series is the quickest workaround).
- **Legacy 3-pin RGB LED**: still supported. Set `#define USE_WS2812 0`
  in `Config.h` to re-enable the GPIO 25 / 26 / 27 PWM mode.

All pins are defined in `Arduino/lasertag_client/Config.h` and can be moved
to any free GPIO (mind ADC2 conflicts when WiFi is active).

---

## Required libraries

Install via the Arduino IDE Library Manager.

| Library | Used in | Purpose |
|---|---|---|
| `IRremoteESP8266` | client | Sends and receives 32-bit NEC IR frames |
| `U8g2` | client | OLED rendering (`U8G2_SSD1306_128X64_NONAME_F_HW_I2C`) |
| `FastLED` | client | WS2812B strip control (only required when `USE_WS2812=1`, default v4.2) |
| `MFRC522` | client | NFC reader (only required when `HAS_NFC=1`) |

Built into the ESP32 core (no install needed): `WiFi`, `esp_now`, `esp_wifi`,
`HTTPClient`, `WiFiClientSecure`, `Preferences`, `ESPmDNS`, `DNSServer`,
`WebServer`.

---

## Upload settings

| Setting | Value |
|---|---|
| Board | ESP32 Dev Module *or* NodeMCU-32S |
| Upload Speed | 115 200 |
| Flash Frequency | 40 MHz |
| Flash Mode | QIO |
| Partition Scheme | Default 4 MB with spiffs |
| Erase all Flash before Sketch Upload | **Yes** |
| Core Debug Level | None (or Verbose during bring-up) |

The "Erase Flash" flag wipes NVS on each upload — important during
development because identity, WiFi credentials and match settings live in
NVS on the server side.

---

## Client (`lasertag_client/`)

### Files

```
lasertag_client/
├── lasertag_client.ino     — setup() + loop()
├── Config.h                — pins, identity, IR codes, timing, phase enums
├── Types.h                 — RankingEntry, Message wire format
├── Globals.h               — extern declarations for shared state
├── Game.cpp/.h             — trigger handling, IR receive, scoring, respawn,
│                             isShootingAllowed() (phase gate)
├── Display.cpp/.h          — OLED renderer (u8g2), phase-aware screens
├── Led.cpp/.h              — RGB status LED (common cathode/anode)
├── Network.cpp + LasertagNetwork.h
│                           — ESP-NOW callbacks, peer mgmt, ranking broadcast
│                             (also handles MSG_PHASE from server in v2)
└── Ranking.cpp/.h          — anti-entropy table, sorting, upsert
```

### Per-device configuration

In `Config.h`, **set a unique identity per ESP**:

```cpp
constexpr char MY_NAME[]      = "PLAYER_1";   // pro Gerät unterschiedlich
constexpr uint8_t MY_IR_COMMAND = 0x01;        // 0x01..0xFE pro Gerät unique
```

The NEC code that gets transmitted on a trigger press is
`encodeNEC(IR_GAME_ADDRESS, MY_IR_COMMAND)` — `IR_GAME_ADDRESS` stays the
same across all devices in a game (default `0x00FF`), `MY_IR_COMMAND` is the
shooter ID. Two devices with the same `MY_IR_COMMAND` will be confused for
each other.

### Match phase awareness (v2)

The client listens for `MSG_PHASE` broadcasts from the server-ESP and
updates its display + shooting gate accordingly:

- `IDLE` / `LOBBY` / `DONE`: triggers ignored, OLED shows the phase
  (LOBBY shows the countdown). Score arrays are reset at the start of
  LOBBY so each match begins fresh.
- `ACTIVE`: normal v1 behaviour (shoot, count hits, gossip the table).

If no phase message arrives for `PHASE_TIMEOUT_MS` (5 s), the client falls
back to stand-alone mode: every phase is treated as ACTIVE. This makes the
client compatible with v1 deployments and lets a single-set demo run
without the server-ESP.

### Stand-alone mode

If you only have player units (no bridge ESP and no internet), the client
firmware works on its own. ESP-NOW peer-to-peer keeps the ranking in sync,
the OLED shows your score and rank. The web portal won't see the match,
but the game itself plays normally.

---

## Server (`lasertag_server/`)

### Files

```
lasertag_server/
├── lasertag_server.ino    — setup() + loop()
├── Config.h               — channel, portal URL, AP prefix, defaults
├── Types.h                — wire format (binary-compatible with client),
│                            MatchPhase enum, MatchSettings struct
├── Globals.h              — extern declarations + identity + match state
├── Storage.cpp/.h         — Preferences (NVS) wrapper
├── Identity.cpp/.h        — random name+PIN generator, NVS-persisted
├── WiFiSetup.cpp/.h       — AP+STA dualmode, WLAN scan, mDNS startup,
│                            reconnect watchdog
├── Portal.cpp/.h          — local web UI (Captive Portal + LAN), all
│                            endpoints PIN-gated
├── EspNow.cpp/.h          — ESP-NOW listener (table diff → hits) +
│                            sender (MSG_PHASE broadcast)
├── Bridge.cpp/.h          — HTTPS client to the web portal:
│                            register, heartbeat, match/start, match/end,
│                            hit forward, settings pull
└── Match.cpp/.h           — state machine IDLE→LOBBY→ACTIVE→DONE
```

### First boot

1. ESP comes up, generates name (`SteelHawk-A4F2`) and PIN (`4815`) at
   random, persists them in NVS.
2. **AP stays up permanently** (`hmyLaser32-XXXX`, open) — there is no
   "STA-only" mode. The captive portal is always reachable via
   `http://192.168.4.1`, even after the ESP joined your home WiFi.
3. If WiFi credentials are stored, the server tries an STA connect for
   20 s. On success: mDNS hostname `<name>.local` is published in your
   LAN, the server tries to register with the web portal.
4. On the captive portal page you see your name, PIN, both the AP and the
   STA IP, the mDNS hostname, and the current match phase.

### Why AP+STA permanently?

So you can fix things even without the home network. The downside is that
ESP32 can only operate AP and STA on the *same* channel — when STA joins a
WiFi on channel 6 the AP also moves to channel 6, which is fine for the
phone connecting via the AP but matters for ESP-NOW (see below).

### Match flow

- Match start can be triggered locally (`http://192.168.4.1/`,
  PIN-gated form) **or** remotely from the web portal
  (`/esp/<id>` → tab Einstellungen → Match starten, PIN-gated).
- The web-portal trigger sets a `startRequested` flag on the server
  record; the ESP pulls the flag every 30 s and on next pull begins the
  LOBBY phase. The flag is consumed atomically on read.
- During LOBBY: server broadcasts `MSG_PHASE` every 1.5 s; clients show a
  countdown.
- Lobby end → server posts `match/start` to the portal, transitions to
  ACTIVE, the broadcast picks up the new phase + remaining time.
- ACTIVE: ESP-NOW table diffs detect new hits; the server forwards each
  point delta to `/api/bridge/hit` so the web feed updates live.
- Match timer expires → server posts `match/end` with the aggregated
  ranking → DONE; clients show end screen.

### Identity reset

When you've forgotten the PIN: on the local web UI, "Reset Name + PIN" with
the current PIN. If you've lost the current PIN too, re-flash with **Erase
all Flash before Upload** turned on — this wipes NVS and a fresh identity
is created on next boot.

### Forget WiFi

If you want to take the server elsewhere: "WLAN-Login löschen" with PIN.
The ESP wipes credentials and reboots; the AP comes up empty and you can
go through the setup again.

---

## ESP-NOW wire format

Defined in `Types.h` on both sides — **must stay binary-compatible**.

```cpp
struct __attribute__((packed)) RankingEntry {
  uint32_t playerId;
  char     player[12];
  int16_t  points;
  uint32_t lastUpdate;
};

struct __attribute__((packed)) Message {
  char        senderName[12];
  uint8_t     msgType;          // 0 DISCOVERY | 1 TABLE | 2 PHASE
  uint8_t     playerCount;
  RankingEntry entries[MAX_PLAYERS];  // MAX_PLAYERS = 10
};
```

### Message types

| Type | Sender | Payload semantics |
|---|---|---|
| `MSG_DISCOVERY` (0) | client | `entries[]` unused. Peer announces presence, others reply with their TABLE. |
| `MSG_TABLE` (1) | client | Full ranking. Receivers run anti-entropy upsert. |
| `MSG_PHASE` (2) | **server** | Reuses `entries[0]`: `.playerId` = phase enum, `.points` = seconds left, `.player` = mode name. |

### Anti-entropy ranking

Each entry has a `lastUpdate` timestamp (the sender's `millis()`). On
receiving a TABLE, a peer compares timestamp + points and upserts only if
the incoming entry is newer or strictly higher-scoring for the same
`playerId`. After any change the peer immediately rebroadcasts. The
periodic timer broadcasts the full table every 10 s as a safety net.

---

## Match state machine

Implemented in `lasertag_server/Match.cpp`:

```
                         ┌──────────────────┐
                         │      IDLE        │ ◄────────┐
                         └────────┬─────────┘          │
                  matchStart()   ▼                     │
                         ┌──────────────────┐          │
                         │     LOBBY        │          │
                         │ verteilen +      │          │
                         │ verstecken       │          │
                         └────────┬─────────┘          │
                  lobby timer    ▼  bridgeStartMatch() │
                         ┌──────────────────┐          │
                         │     ACTIVE       │          │
                         │ hits forwarded   │          │
                         └────────┬─────────┘          │
                  match timer    ▼                     │
                  | abort()      ▼ bridgeEndMatch()    │
                         ┌──────────────────┐          │
                         │      DONE        │ ─────────┘ matchStart()
                         └──────────────────┘
```

`MSG_PHASE` is rebroadcast every 1.5 s with the current phase and remaining
seconds so newly joining clients catch up.

---

## Debugging & serial logs

Both sketches log to `Serial` at 115 200 baud. Useful prefixes to grep for:

| Prefix | Meaning |
|---|---|
| `[ESP-NOW] Peer added:` | New broadcast peer learned |
| `[ESP-NOW] Discovery from` | A peer announced itself |
| `[ESP-NOW] Ranking update from` | Table changed because of a remote score |
| `[ESP-NOW] Send status:` | TX result (Success/Fail) |
| `[PHASE]` | Phase change on the client (e.g. `0 -> 1 (free-for-all, 60 s)`) |
| `[BUTTON]` | Raw button transitions |
| `[SHOT]` | Trigger pressed + IR code sent |
| `[IR]` | IR receive events |
| `[GAME]` | Score / respawn updates |
| `[STATS]` | One-line status dump |
| `[bridgeRegister/match/hit]` | Server's HTTPS bridge with the portal |
| `[matchLoop]` | State machine transitions on the server |

If something is silent: check that `WIFI_CHANNEL` matches across all
clients **and** the server (the server prints the active channel on the
captive portal page after STA-connect).

---

## Extending the firmware

### Adding a new message type

1. Pick a new `MSG_*` constant (e.g. `MSG_HIT_DETAIL = 3`) and add it to
   `Types.h` on **both** sketches.
2. Reuse the `entries[]` slots for your payload — do **not** change the
   struct size or you break wire compatibility with older units.
3. Handle the new type in `Network.cpp::processIncomingMessage` (client)
   and `EspNow.cpp::onRecv` (server).
4. Document the slot semantics next to the `MSG_*` constant.

### Adding a new bridge endpoint

1. Add the route on the web side under `src/app/api/...` and document the
   contract.
2. Implement the client in `lasertag_server/Bridge.cpp`, exposing a small
   helper function declared in `Bridge.h`.
3. Wire it into `lasertag_server.ino`'s main `loop()` if it needs to run
   periodically, or hook it into `Portal.cpp` if user-triggered.

### Adding an OLED to the server

The server is headless by default. The client's `Display.cpp` is a good
template. Two recommended fields:

- A boot screen showing name + PIN until the user has noted them.
- Once connected: name + PIN + STA IP + match phase.

You'll also want to feed `Match::matchLoop()` calls back into the OLED
refresh.

### Pinning the TLS root CA

Replace `client.setInsecure()` in `Bridge.cpp::makeClient` with
`client.setCACert(rootCa)` where `rootCa` is a PEM-encoded constant string
containing the Cloudflare Origin CA (or Let's Encrypt ISRG Root X1,
depending on the portal cert chain).

---

## Notes for AI agents

- The pin numbers in `Config.h` are the authoritative source. When the
  README and the code disagree, fix the README.
- The wire format struct is **binary-shared** between client and server.
  Adding/removing fields needs a coordinated change across both sketches
  and an explicit version bump in `MSG_*` constants — old units must keep
  parsing newer payloads without crashing (the default branch already
  logs and ignores unknown `msgType`).
- The local web UI on the server is a single C++ string in `Portal.cpp` to
  avoid the SPIFFS round-trip. If you want richer UI, prefer hardcoding
  HTML+CSS+JS in that string over adding a filesystem dependency — it
  keeps the firmware self-contained.
- Identity (`name` + `pin`) is the only persistent piece of identification.
  Never put the PIN in `Serial.print` once production; for debugging it's
  OK because the device is offline anyway.
- Long blocking calls (`WiFi.scanNetworks(false, …)` in `wifiScanJson`) are
  acceptable inside captive-portal requests but not in `loop()`. Keep
  `loop()` tight so ESP-NOW callbacks fire on time.
