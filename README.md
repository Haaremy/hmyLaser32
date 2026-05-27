# hmyLaser32 - DIY Lasertag

A self-hosted ESP32 Lasertag system with a local ESP server, ESP-NOW player
clients, NFC player binding, and an optional Next.js web portal for profiles,
live statistics, and match history.

- Live portal: <https://laser32.haaremy.de>
- Paper V1: [`Lasertag_Paper.pdf`](./Lasertag_Paper.pdf)
- Firmware docs: [`Arduino/README.md`](./Arduino/README.md)
- Web portal notes: [`README_WEBSITE.md`](./README_WEBSITE.md)

## Current Status

The repository contains:

- Player firmware for ESP32 clients with IR shooting, two IR receiver zones,
  OLED display, WS2812B strip, optional NFC login, and ESP-NOW sync.
- Server firmware for an ESP32 bridge with permanent AP + captive portal,
  WLAN setup, local game administration, ESP-NOW orchestration, and HTTPS
  forwarding to the web portal.
- A Next.js 14 web portal with Prisma/PostgreSQL for registered players,
  ESP servers, live matches, leaderboards, and account/NFC management.

## Architecture

```text
Player ESPs
  - IR NEC shots
  - local hit detection
  - ESP-NOW player state, hit events, ranking gossip
        |
        | ESP-NOW broadcast
        v
ESP Server
  - permanent AP/captive portal
  - WLAN STA bridge
  - match phase authority
  - player color/team sync
  - live stats aggregation
        |
        | HTTPS Bearer PIN
        v
Next.js Portal + PostgreSQL
  - server registration
  - live match view
  - player/NFC accounts
  - historical stats
```

## Repository Layout

```text
.
|-- Arduino/
|   |-- README.md
|   |-- lasertag_client/   # player ESP sketch
|   `-- lasertag_server/   # bridge/server ESP sketch
|-- prisma/schema.prisma   # User, EspServer, Match, MatchPlayer, Team, HitEvent
|-- src/                   # Next.js app router, API routes, UI
|-- public/                # static assets
|-- server.mjs             # custom HTTP + WebSocket entry
|-- Dockerfile
|-- docker-compose.yml
|-- README_WEBSITE.md
`-- Lasertag_Paper*.*
```

## Local ESP Server Portal

The ESP server opens an AP named `hmyLaser32-XXXX` and serves the captive
portal at `http://192.168.4.1`. The AP stays available even after the server
joins a configured WLAN.

The local portal now has two tabs:

- `Spiel`: operational match control.
  - `Warteraum starten`
  - distribution time and match duration in seconds
  - points per hit
  - match mode dropdown: `Alle gegen Alle` or `Team-Modus`
  - known player table with color, device ID, name, and team
  - `Aktualisieren` syncs player color/team assignments to clients
  - `Match starten` activates a lobby match
  - live table: `Rank | Spieler | Shots | RX Hits | Punkte`
- `Einstellungen`: server info and WLAN setup only.
  - server name/PIN
  - AP and STA information
  - WLAN scan/save/forget

There is intentionally no early match abort button.

## Firmware Data Flow

The shared ESP-NOW `Message` struct remains binary-compatible. New behavior is
implemented through additional `MSG_*` message types instead of enlarging the
wire struct.

Important message types:

| Type | Direction | Purpose |
|---|---|---|
| `MSG_DISCOVERY` | client -> all | announce presence, optionally with self entry |
| `MSG_TABLE` | client -> all | legacy ranking table sync |
| `MSG_PHASE` | server -> clients | match phase, remaining seconds, mode, hit points |
| `MSG_TEAMS` | server -> clients | team colors and member bitmasks |
| `MSG_NFC` | client -> server | scanned `name + UUID` chunks |
| `MSG_STANDALONE` | client -> clients | no-server lobby consensus |
| `MSG_PLAYER_STATE` | client -> server | name, points, shots, RX hits, color, team, NFC token |
| `MSG_PLAYER_CONFIG` | server -> clients | assigned name/color/team from portal |
| `MSG_HIT_EVENT` | client -> server | shooter, target, points, receiver stats |

Players become known to the server when they appear in discovery/table/state
messages. The ESP server does not persist players; it only persists WLAN,
identity, and local match defaults.

## Game Rules

- A valid hit gives the shooter `hitPoints` points, default `10`.
- `hitPoints` can be changed in the ESP local portal and in the web settings.
- `Shots` is the number of trigger pulls sent by a client.
- `RX Hits` is the number of hits received by a client.
- Team-mode friendly fire is ignored.
- Free-for-all uses each player's own color/team number mainly as display data.
- Team mode can use up to 10 team slots; when one player color in a team is
  changed locally, the server applies that color to all players in that team.

## NFC Binding

Optional NFC cards use this payload format:

```text
username|uuid-token
```

When scanned, the client:

- changes its local display name to `username`
- sends the full token in ESP-NOW chunks to the server
- continues sending that name/token in player state updates

The local ESP portal displays the name only. The UUID is forwarded to the web
portal so it can link match data to registered accounts.

## Web Portal Quick Start

```bash
npm install
cp .env.example .env
# Fill DATABASE_URL and SESSION_SECRET. SESSION_SECRET must be >= 32 chars.
npx prisma generate
npx prisma migrate deploy
npm run build
node server.mjs
```

During local development without migrations you can use:

```bash
npx prisma db push
npm run dev
```

The current schema includes `EspServer.hitPoints`. Existing databases must be
migrated or pushed before the updated app can use the field.

## Firmware Quick Start

Use Arduino IDE 2.x or PlatformIO with the Espressif ESP32 Arduino core.

Required client libraries:

- `IRremoteESP8266`
- `U8g2`
- `FastLED`
- `MFRC522` only when `HAS_NFC=1`

Open and upload:

```text
Arduino/lasertag_client/lasertag_client.ino
Arduino/lasertag_server/lasertag_server.ino
```

For development, enable "Erase all Flash before Upload" when you need to wipe
stored WLAN credentials, server identity, PIN, or match defaults.

## Bridge API Overview

Bridge endpoints authenticate with:

```http
Authorization: Bearer <server-pin>
```

| Method | Endpoint | Purpose |
|---|---|---|
| `POST` | `/api/bridge/register` | server self-registration |
| `GET` | `/api/bridge/register` | heartbeat |
| `POST` | `/api/bridge/match/start` | create active match |
| `POST` | `/api/bridge/match/end` | finish match and upload final stats |
| `POST` | `/api/bridge/hit` | live hit event |
| `POST` | `/api/bridge/players` | known/live player state |
| `GET` | `/api/bridge/player/[nfcToken]` | NFC account lookup |
| `GET` | `/api/esp/by-pin/settings` | ESP pulls defaults and start requests |

## Verification

The web project currently builds with:

```powershell
$env:SESSION_SECRET='0123456789abcdef0123456789abcdef'
npm run build
```

`arduino-cli` is not part of this repository; firmware compilation should be
verified in Arduino IDE or PlatformIO after changing ESP32 dependencies.

## Notes For Contributors

- Keep `Types.h` binary-compatible between client and server.
- Prefer new `MSG_*` types over growing the ESP-NOW struct.
- Do not persist player rosters on the ESP server; the web portal is the
  long-term statistics store.
- The server PIN is the bridge secret. Do not put it into URLs.
- The ESP32 AP and STA share one WiFi channel. Clients auto-detect the server
  AP where possible, but channel mismatch remains the first thing to check when
  ESP-NOW seems silent.

## License

Code in this repository is open source. The papers are copyright Jeremy Becker,
Hochschule Anhalt. Please do not republish the PDFs without permission.
