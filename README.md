# hmyLaser32 — DIY Lasertag

A self-hosted Lasertag system built around the ESP32 with an online portal for
profiles, statistics and live spectating.

- 🎯 **Live**: <https://laser32.haaremy.de>
- 📄 **Paper V1** (final design): [`Lasertag_Paper.pdf`](./Lasertag_Paper.pdf)
- 📝 **Paper V2** (extension, in progress)

> **Status (2026-05-16):** Client + Server firmware in `Arduino/`. Both build
> with Arduino IDE 2.x / PlatformIO. Web portal is a Next.js 14 app deployed
> on Proxmox LXC 128/129 behind a Cloudflare-proxied vhost on LXC 124.

---

## Table of contents

1. [What is it?](#what-is-it)
2. [Architecture](#architecture)
3. [Repository layout](#repository-layout)
4. [Quick start — players](#quick-start--players)
5. [Quick start — developers](#quick-start--developers)
6. [Firmware](#firmware)
7. [Web portal](#web-portal)
8. [Bridge API reference](#bridge-api-reference)
9. [State of the project / current limitations](#state-of-the-project)
10. [Notes for AI agents](#notes-for-ai-agents)

---

## What is it?

A Lasertag set replicates the indoor-arena experience with cheap hardware
(~€25 per player). Each player carries:

- a **pistol** with an IR-LED that fires a coded NEC pulse on a trigger press
- a **chest module** with an IR receiver, an OLED status display and an RGB
  status LED

A separate **bridge ESP** acts as a server that orchestrates matches and
forwards live data to the web portal. The portal records players, statistics
and a live feed. The client firmware also works **completely stand-alone**
(no server, no internet) — in that case the ESP-NOW Gossip protocol keeps the
score tables in sync between the player units.

---

## Architecture

```
                                ┌─────────────────────────┐
                                │  laser32.haaremy.de     │
                                │  (Next.js + Postgres)   │
                                └────────────┬────────────┘
                                             │  HTTPS (Bearer = PIN)
                            HTTP register / hit / match start+end
                                             │
                                ┌────────────┴────────────┐
                                │   ESP-Server (bridge)   │
                                │  • Captive Portal AP    │
                                │  • STA → home WiFi      │
                                │  • mDNS hmylaser32.local│
                                │  • ESP-NOW listener     │
                                │  • Match state machine  │
                                └────────────┬────────────┘
                                             │
                                ESP-NOW broadcast (Channel 1)
                                ┌────────────┴────────────┐
                                │                         │
                       ┌────────┴───────┐         ┌──────┴─────────┐
                       │  Client ESP #1 │  …      │  Client ESP #N │
                       │  • IR sender   │         │  • OLED        │
                       │  • IR receiver │         │  • RGB-LED     │
                       │  • Trigger btn │         │  • Button      │
                       └────────────────┘         └────────────────┘
                              │   IR-NEC pulses                ▲
                              └────────  fire ────────────────►
```

**Three communication paths**:

1. **IR-NEC** (player ↔ player): trigger sends a 32-bit NEC code carrying the
   shooter's player ID. The receiving player parses it, awards points to the
   shooter and goes into a 5-second respawn lock.
2. **ESP-NOW Gossip** (peer-to-peer + server-broadcast): every client
   periodically rebroadcasts its full ranking table; the server listens and
   diffs the tables to detect new hits. The server additionally broadcasts
   `MSG_PHASE` with the current match phase so clients can show a countdown.
3. **HTTPS Bridge** (server ↔ web portal): the server registers itself,
   forwards hit events during an active match, posts the final stats and
   pulls match settings/start requests from the portal.

---

## Repository layout

```
.
├── Arduino/
│   ├── README.md                ← detailed firmware docs
│   ├── lasertag_client/         ← player ESP sketch (modular)
│   └── lasertag_server/         ← bridge ESP sketch (modular)
│
├── prisma/schema.prisma         ← User, EspServer, Match, MatchPlayer,
│                                  Team, HitEvent
├── src/                         ← Next.js 14 app (App Router, TypeScript)
├── public/                      ← static assets + hmyDesign tokens
├── server.mjs                   ← custom Next entry (HTTP + WebSocket)
├── Dockerfile + docker-compose  ← self-hosted deploy template
├── Lasertag_Paper.pdf           ← Paper V1 (final design)
├── Lasertag_Paper_V1.zip        ← LaTeX sources for V1
└── Lasertag_Paper_Erweiterung*  ← V2 sources (in progress)
```

See [`Arduino/README.md`](./Arduino/README.md) for the firmware module
breakdown and protocol specifics.

---

## Quick start — players

1. Build a player set following the [DIY cookbook](https://laser32.haaremy.de/diy)
   on the web portal — same content lives in [`Arduino/README.md`](./Arduino/README.md).
2. Flash `lasertag_client/` to each player ESP, setting a unique `MY_NAME`
   and `MY_IR_COMMAND` per device.
3. Flash `lasertag_server/` to a dedicated bridge ESP, power it via a
   powerbank — at first boot it opens an **open AP** `hmyLaser32-XXXX`.
4. Connect a phone to the AP, the captive portal opens at `192.168.4.1`.
5. Scan for your home WLAN, enter the password, save.
6. The ESP reboots, connects, registers itself at
   <https://laser32.haaremy.de> and appears on the landing page.
7. Open the server's card on the portal → tab **Einstellungen** → enter the
   PIN shown on the captive portal → click **Match jetzt starten**.
8. A 60-second lobby phase starts (visible on the client OLEDs); after that
   the active match runs.

> The captive-portal AP stays available **permanently** — even after the
> ESP has joined your home WiFi. You can always come back to
> `http://192.168.4.1` from a phone connected to `hmyLaser32-XXXX`, or via
> `http://hmylaser32-<name>.local/` from inside your home WiFi.

---

## Quick start — developers

```bash
# Web portal
npm install
cp .env.example .env             # fill DATABASE_URL, SESSION_SECRET, …
npx prisma migrate deploy
npm run build
node server.mjs                  # custom entry, serves HTTP + WS on :3000

# Or via docker-compose
docker compose up --build
```

```bash
# Firmware (Arduino IDE)
File → Open → Arduino/lasertag_client/lasertag_client.ino
File → Open → Arduino/lasertag_server/lasertag_server.ino
Tools → Board → ESP32 → ESP32 Dev Module (or NodeMCU-32S)
Tools → Upload Speed → 115200
Tools → Erase all Flash before Upload → Yes
Sketch → Upload
```

Required libraries (install via Library Manager):

| Library | Used in |
|---|---|
| `IRremoteESP8266` | client (IR send/recv NEC) |
| `U8g2` | client (OLED) |
| `Preferences` | server (NVS — already part of esp32 core) |
| `ESPmDNS` | server (mDNS — already part of esp32 core) |

---

## Firmware

Full reference: [`Arduino/README.md`](./Arduino/README.md).

| Sketch | Role | Hardware |
|---|---|---|
| `lasertag_client` | Player. IR pistol + chest receiver + OLED + RGB-LED + button | ESP32 + TSOP38238 + KY-005 + SSD1306 + 5mm RGB common-cathode + 330Ω × 3 + powerbank |
| `lasertag_server` | Bridge. AP+STA, captive portal, mDNS, ESP-NOW listener, HTTPS bridge | ESP32 + powerbank (no IR hardware) |

**Match state machine (server-side):**

```
IDLE  ──(matchStart, web or local)──►  LOBBY (60 s, broadcast countdown)
LOBBY ──(lobby timer)──►  ACTIVE (matchSeconds, hits are forwarded)
ACTIVE ──(match timer | abort)──►  DONE (stats posted to portal)
DONE  ──(matchStart)──►  LOBBY
```

The phase is broadcast over ESP-NOW (`MSG_PHASE`) every ~1.5 s so clients
can render a countdown and block shots outside the active window. Clients
fall back to standalone behaviour (all phases treated as ACTIVE) if no
phase message arrives within 5 s — the original v1 paper design.

---

## Web portal

Next.js 14 App Router + TypeScript + Prisma + PostgreSQL + iron-session +
argon2 + next-intl (DE/EN).

| Route | Purpose |
|---|---|
| `/` | List of ESP servers, online state, active match |
| `/research` | Paper V1 + V2 cards with download + read-online |
| `/research/paper-v1` | Full web view of the V1 paper |
| `/diy` | Cookbook build guide (ingredients, tools, 7 numbered steps) |
| `/champions` | Global leaderboard with filter: Overall / Hits / Matches / K/D |
| `/account` | Login/Register tabs, after login: profile + NFC token + stats |
| `/esp/[id]` | Server overview + settings tab (PIN-gated) + Match starter |
| `/match/[id]` | Live view: countdown timer, leaderboard, feed; settings tab |

The portal follows the hmyDesign system: component classes are `hmy-btn`,
`hmy-card__header`, `hmy-tabs`, `hmy-input` etc., and Lasertag-specific
extensions are prefixed `hmy-lt-*`.

---

## Bridge API reference

All endpoints accept JSON. Bridge endpoints authenticate with
`Authorization: Bearer <pin>` — the PIN is generated by the ESP server on
first boot and shown on its captive portal page.

| Method | Endpoint | Auth | Body / Use |
|---|---|---|---|
| `POST` | `/api/bridge/register` | — | `{ name, pin }` — self-register. `200` rebind, `201` new, `409` conflict |
| `GET`  | `/api/bridge/register` | Bearer | heartbeat |
| `POST` | `/api/bridge/match/start` | Bearer | `{ mode, durationSeconds, teams? }` → `{ matchId }` |
| `POST` | `/api/bridge/match/end` | Bearer | `{ matchId, players[] }` |
| `POST` | `/api/bridge/hit` | Bearer | `{ matchId, shooterNfc?, targetNfc?, points }` |
| `GET`  | `/api/bridge/player/[nfcToken]` | Bearer | look up player by NFC |
| `GET`  | `/api/bridge/ws` | upgrade | WebSocket; `?token=<pin>` |
| `GET`  | `/api/esp/by-pin/settings` | Bearer | pull match defaults + `startRequested` flag (consumed atomically) |
| `GET`  | `/api/esp/[id]/settings` | public | read server defaults |
| `POST` | `/api/esp/[id]/settings` | body.pin | edit defaults |
| `POST` | `/api/esp/[id]/start` | body.pin | request a match start (ESP picks it up on next pull) |
| `GET`  | `/api/match/[id]/live` | public | live state + leaderboard + feed |
| `GET`  | `/api/match/[id]/settings` | public | per-match settings |
| `POST` | `/api/match/[id]/settings` | body.pin | edit per-match settings |

---

## State of the project

### Working

- ✅ Player client (v2) — IR pistol, OLED, RGB-LED, ESP-NOW gossip, phase-aware
- ✅ Server (v2) — AP+STA dualmode, captive portal, WLAN scan with persistence,
  mDNS, ESP-NOW listener, HTTPS bridge, match state machine, settings pull
- ✅ Web portal — landing page, research, DIY, account, champions, match,
  ESP detail with PIN-gated settings + Match-Start
- ✅ Live match smoke test against Test-ESP `BlueWolf-7` (PIN `4815`)
- ✅ Self-hosted on LXC 128 (app, 10.0.3.50) + LXC 129 (Postgres, 10.0.3.51)
  behind LXC 124 (Apache reverse-proxy) at `laser32.haaremy.de`

### Known limitations

- **TLS pinning**: the firmware uses `WiFiClientSecure::setInsecure()` for
  the HTTPS connection to the portal. Production-ready setup needs the
  Cloudflare root CA pinned (`setCACert`). Not a problem for a LAN demo.
- **Channel mismatch**: ESP-NOW runs on the channel the WiFi STA picks. If
  your home router uses anything but channel 1, set `WIFI_CHANNEL` in the
  client to match the channel the server actually ends up on (visible on
  the captive-portal page after STA-connect).
- **Score-diff hit forwarding**: the server detects hits by diffing the
  per-player point totals broadcast by clients. It cannot reliably tell who
  the *target* was — that information is local to the receiving client. A
  future client extension (v3) should add a `hit_event` message with both
  shooter and target.
- **No enclosure** in V1 — components sit on perfboard, wires are visible.
  V2 paper covers mechanical design.

### Roadmap

- 🛠 Client v3: send `hit_event` with explicit `targetNfc`.
- 🛠 Server v3: optional OLED + button extension.
- 📦 Mechanical design (enclosures, wearable rig) — Paper V2 deliverable.
- 🔐 TLS pinning in the bridge HTTPS client.
- 📲 Companion mobile app for spectator-mode push.

---

## Notes for AI agents

The code is structured for easy navigation and modification:

- **Every module has a focused responsibility.** Don't add cross-module
  state — keep new state in `Globals.h` (firmware) or `lib/` (web).
- **Wire format is shared between client and server**: `Types.h` in both
  Arduino sketches *must* stay binary-compatible. If you add fields, do it
  via `MSG_*` constants and reuse the `entries[]` slots, never extend the
  struct.
- **Adding a new bridge endpoint**: add the route in `src/app/api/...`, mirror
  the contract in `Arduino/lasertag_server/Bridge.{h,cpp}`, document it in
  the [Bridge API table](#bridge-api-reference).
- **CSS conventions**: stick to hmyDesign tokens (`var(--hmy-color-...)`) and
  the BEM class naming (`hmy-card__header`, `hmy-btn--primary`). Local extensions
  use the `hmy-lt-*` prefix to keep them grep-able.
- **PIN handling**: the ESP-Server PIN is the *only* secret on the device.
  Never log it, never put it in URLs.
- **Channel `1` is hard-coded** in the client. Match it on the server side
  via `WIFI_CHANNEL` in `Arduino/lasertag_server/Config.h`. When the server
  joins a STA WiFi on a different channel, ESP-NOW follows the STA channel
  — both have to align.
- **Test-ESP**: `BlueWolf-7` with PIN `4815` is registered in the production
  database for smoke testing. Delete it when going live.

---

## License

The code in this repository is open source. The papers are © Jeremy Becker,
Hochschule Anhalt. Please don't republish the PDFs without permission.
