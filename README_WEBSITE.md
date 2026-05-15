# hmyLaser32 Portal — Website Agent Brief

## Project Overview

Build a web portal for the **hmyLaser32** Lasertag system. The website serves two purposes:

1. **Online Bridge / Webservice** — receives match data from an ESP32 server and provides player profiles, statistics, and leaderboards
2. **DIY Wiki** — hardware diagrams, architecture docs, build guide, and component list so others can replicate the system

The underlying hardware system uses ESP32 microcontrollers communicating via ESP-NOW (peer-to-peer, no router needed). An ESP32 server acts as the local truth instance coordinating game state. This website is the optional online extension described in the project papers.

GitHub repository: `https://github.com/Haaremy/hmyLaser32`

---

## Tech Stack

| Layer | Choice |
|---|---|
| Framework | Next.js 14+ (App Router) with TypeScript |
| Database | PostgreSQL via Prisma ORM |
| Auth | Custom session system (no SSO) — Iron Session or similar |
| Styling | hmyDesign (you know this design system) |
| i18n | `next-intl` — bilingual DE/EN with language switcher in the header |
| Hosting | Self-hosted; provide `Dockerfile` + `docker-compose.yml` |

---

## Pages & Routing

| Route | Description |
|---|---|
| `/` | Landing page — project intro, GitHub link, feature overview |
| `/wiki` | DIY Wiki overview |
| `/wiki/hardware` | Wiring diagrams: ESP32 client (IR sender/receiver, OLED, battery) and ESP32 server |
| `/wiki/architecture` | Software architecture diagrams: ESP-NOW local flow, online bridge communication model |
| `/wiki/build-guide` | Step-by-step build guide with image placeholders |
| `/wiki/components` | Component and shopping list table |
| `/auth/register` | Registration (username + password) |
| `/auth/login` | Login |
| `/profile` | Own player profile: NFC token display, stats, match history |
| `/leaderboard` | Global leaderboard |
| `/matches/[id]` | Match detail view |
| `/admin` | Admin panel (admin-only) |
| `/admin/users` | User management: create, view, suspend, delete accounts |
| `/admin/devices` | ESP32 server management and access code generation |
| `/admin/matches` | All matches overview, filter by date/server |
| `/game` | Game panel: per-player/per-match statistics and CSV/JSON export |

---

## Authentication & NFC Integration

- Registration: username + password (no email required)
- On registration, a **UUID NFC token** is generated and attached to the account
- The player writes `username + nfcToken` to an NFC card; the ESP32 system uses this card to identify players (display name + account linkage)
- Login: username + password (session-based)
- The NFC token is visible in the user's profile page (for card setup)
- Admin role: boolean/enum flag in the database

---

## Database Schema

```prisma
model User {
  id           String   @id @default(cuid())
  username     String   @unique
  passwordHash String
  nfcToken     String   @unique @default(uuid())
  role         Role     @default(USER)
  createdAt    DateTime @default(now())
  matches      MatchPlayer[]
}

enum Role { USER ADMIN }

model EspServer {
  id         String   @id @default(cuid())
  name       String
  accessCode String   @unique
  lastSeen   DateTime?
  ownerId    String?
  matches    Match[]
}

model Match {
  id              String   @id @default(cuid())
  serverId        String
  server          EspServer @relation(fields: [serverId], references: [id])
  startedAt       DateTime
  endedAt         DateTime?
  durationSeconds Int?
  mode            String?
  status          String   @default("active")
  players         MatchPlayer[]
  teams           Team[]
}

model MatchPlayer {
  id       String  @id @default(cuid())
  matchId  String
  match    Match   @relation(fields: [matchId], references: [id])
  userId   String?
  user     User?   @relation(fields: [userId], references: [id])
  nfcToken String
  teamId   String?
  hits     Int     @default(0)
  deaths   Int     @default(0)
  points   Int     @default(0)
}

model Team {
  id          String @id @default(cuid())
  matchId     String
  match       Match  @relation(fields: [matchId], references: [id])
  name        String
  color       String
  totalPoints Int    @default(0)
}
```

---

## API — Online Bridge (ESP32 ↔ Website)

All bridge endpoints authenticate via `Authorization: Bearer <accessCode>` header (the access code is generated in `/admin/devices` and entered into the ESP32 Captive Portal).

### REST Endpoints

| Method | Endpoint | Description |
|---|---|---|
| POST | `/api/bridge/match/start` | Start a match; returns `matchId` |
| POST | `/api/bridge/match/end` | End a match and upload final statistics |
| POST | `/api/bridge/hit` | Send a single hit event |
| GET | `/api/bridge/player/:nfcToken` | Fetch player info (username, team history) by NFC token |

**POST `/api/bridge/match/start`**
```json
{ "serverName": "Server-1", "mode": "team-deathmatch", "durationSeconds": 600, "teams": [{"name": "Red", "color": "#ff0000"}, {"name": "Blue", "color": "#0000ff"}] }
```

**POST `/api/bridge/match/end`**
```json
{ "matchId": "...", "players": [{"nfcToken": "...", "teamName": "Red", "hits": 5, "deaths": 2, "points": 150}] }
```

### WebSocket

Endpoint: `/api/bridge/ws`

- The ESP32 server connects for live data streaming during an active match
- Events sent by ESP32: `hit_event`, `timer_sync`, `game_state_update`
- Browser clients on `/game` can subscribe to live match updates via the same WebSocket namespace

---

## Admin Panel

### Webservice Admin (`/admin`)

- Create, view, suspend, delete user accounts
- Manually reset or reassign NFC tokens
- View all matches with filters (date, server, status)

### Game Admin (`/admin/devices`)

- List registered ESP32 servers (name, last online, access code)
- Generate new access codes (one code = one server)
- The user copies this code into the ESP32 Captive Portal → server becomes linked to the portal

### Game Panel (`/game`)

- Per-player statistics: matches played, total hits, total deaths, K/D ratio, points
- Per-match detail with team breakdown
- Export: CSV and JSON download of match data

---

## DIY Wiki Content

Populate all wiki pages with placeholder content and a clear structure. The actual diagrams and images will be filled in later. Use placeholder `<img>` tags with descriptive `alt` text and a note "Diagram coming soon."

**Hardware page**: Schematic placeholders for:
- ESP32 Client: IR LED (940nm), TSOP receiver, 0.96" OLED display, LiPo battery circuit
- ESP32 Server: same ESP32 base, WiFi AP mode diagram

**Architecture page**: Communication flow diagram:
- ESP32 Clients ↔ ESP32 Server (ESP-NOW, local)
- ESP32 Server ↔ Website (WLAN, REST + WebSocket)
- Message types: `GAME_START`, `GAME_END`, `TEAM_ASSIGN`, `HIT_EVENT`, `STATUS`, `SYNC_RESULT`

**Build Guide page**: Numbered steps with image placeholders, e.g.:
1. Flash ESP32 firmware
2. Wire IR sender
3. Wire IR receiver (TSOP)
4. Connect OLED display
5. Assemble housing
6. Configure via Captive Portal

**Components page**: Table with columns `Component | Qty | Notes`

---

## i18n

- Language: German (de) and English (en)
- Use `next-intl` with locale files at `/messages/de.json` and `/messages/en.json`
- Language switcher in the header navbar
- Default locale: `de`
- All page titles, labels, nav items, and wiki content text must be translated

---

## Deployment

Provide:
- `Dockerfile` (multi-stage build, production-optimized)
- `docker-compose.yml` with `app` + `postgres` services
- `.env.example`:

```env
DATABASE_URL=postgresql://user:password@db:5432/hmylaser
SESSION_SECRET=changeme
BRIDGE_ACCESS_SECRET=changeme
NEXT_PUBLIC_GITHUB_URL=https://github.com/Haaremy/hmyLaser32
NEXT_PUBLIC_DEFAULT_LOCALE=de
```

No external CDN dependencies — all assets must be self-hosted (fonts, icons, etc.).

---

## Design Notes

- Use **hmyDesign** for all UI components, colors, and typography
- Responsive, mobile-first (players may access the site from their phones)
- Dark mode support if hmyDesign provides it

---

## Verification Checklist

After implementation verify:

- [ ] `docker-compose up` starts app + DB without errors
- [ ] User can register with username/password, NFC token is generated
- [ ] User can log in and view their NFC token on the profile page
- [ ] `POST /api/bridge/match/end` with valid `accessCode` saves match to DB
- [ ] WebSocket at `/api/bridge/ws` accepts connections
- [ ] `/wiki/*` pages render with placeholder content
- [ ] `/admin` is inaccessible without admin role
- [ ] Language switcher toggles between DE and EN
- [ ] Docker container builds and runs in production mode
