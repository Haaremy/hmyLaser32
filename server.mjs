// Custom Next.js entrypoint with WebSocket support.
//
// WebSocket-Endpoints:
//   /api/bridge/ws         — ESP-Server-Bridge (Bearer = PIN)
//   /api/ws/match/<id>     — Browser-Stream pro Match (live state)
//   /api/ws/esp/<id>       — Browser-Stream pro ESP (settings + players)
//
// Globals.wsHub wird von API-Routes verwendet, um Events nach DB-Mutations
// zu broadcasten (z.B. wenn /api/bridge/hit einen HitEvent persistiert).
import { createServer } from 'node:http';
import { parse } from 'node:url';
import next from 'next';
import { WebSocketServer } from 'ws';
import { PrismaClient } from '@prisma/client';

const dev = process.env.NODE_ENV !== 'production';
const port = parseInt(process.env.PORT || '3000', 10);
const hostname = process.env.HOSTNAME || '0.0.0.0';

const app = next({ dev, hostname, port });
const handle = app.getRequestHandler();
const prisma = new PrismaClient();

await app.prepare();

const httpServer = createServer((req, res) => {
  const parsedUrl = parse(req.url || '/', true);
  handle(req, res, parsedUrl);
});

// --- Channels --------------------------------------------------------------
// matchSubs: matchId  → Set<WebSocket>
// espSubs:   espId    → Set<WebSocket>
// espRooms:  matchId  → Set<WebSocket>  (ESP-Bridge subscribers)
const matchSubs = new Map();
const espSubs = new Map();
const espRooms = new Map();

function addToSet(map, key, ws) {
  if (!map.has(key)) map.set(key, new Set());
  map.get(key).add(ws);
}
function removeFromAllSets(map, ws) {
  for (const [k, set] of map.entries()) {
    if (set.delete(ws) && set.size === 0) map.delete(k);
  }
}
function broadcastSet(map, key, payload) {
  const set = map.get(key);
  if (!set) return;
  const data = JSON.stringify(payload);
  for (const ws of set) if (ws.readyState === 1) ws.send(data);
}

// --- WS-Hub for API routes -------------------------------------------------
globalThis.wsHub = {
  broadcastMatch: (matchId, payload) => broadcastSet(matchSubs, matchId, payload),
  broadcastEsp:   (espId,   payload) => broadcastSet(espSubs,   espId,   payload),
};

// --- ESP-Bridge WS ---------------------------------------------------------
const bridgeWss = new WebSocketServer({ noServer: true });

async function persistHit(matchId, msg) {
  try {
    await prisma.hitEvent.create({
      data: {
        matchId,
        ts: msg.ts ? new Date(msg.ts) : new Date(),
        shooterNfc: msg.shooter || msg.shooterNfc || null,
        targetNfc: msg.target || msg.targetNfc || null,
        points: typeof msg.points === 'number' ? msg.points : 0,
        raw: msg
      }
    });
    // Broadcast to browsers viewing this match
    globalThis.wsHub.broadcastMatch(matchId, { type: 'hit', ts: Date.now() });
  } catch (e) {
    console.error('hit persist error', e?.message);
  }
}

bridgeWss.on('connection', (ws, serverInfo) => {
  ws.serverId = serverInfo.id;
  ws.matchId = null;
  ws.on('message', async (raw) => {
    let msg;
    try { msg = JSON.parse(raw.toString()); } catch { return; }
    if (msg.type === 'subscribe' && typeof msg.matchId === 'string') {
      ws.matchId = msg.matchId;
      addToSet(espRooms, msg.matchId, ws);
      ws.send(JSON.stringify({ type: 'subscribed', matchId: msg.matchId }));
      return;
    }
    if (msg.type === 'hit_event' && typeof msg.matchId === 'string') {
      await persistHit(msg.matchId, msg);
      broadcastSet(espRooms, msg.matchId, { type: 'hit_event', ...msg });
    }
    if (msg.type === 'timer_sync' && typeof msg.matchId === 'string') {
      broadcastSet(espRooms, msg.matchId, { type: 'timer_sync', ...msg });
    }
    if (msg.type === 'game_state_update' && typeof msg.matchId === 'string') {
      broadcastSet(espRooms, msg.matchId, { type: 'game_state_update', ...msg });
    }
  });
  ws.on('close', () => removeFromAllSets(espRooms, ws));
  ws.send(JSON.stringify({ type: 'hello', ts: Date.now() }));
});

// --- Browser-Match WS ------------------------------------------------------
const matchWss = new WebSocketServer({ noServer: true });

async function buildMatchSnapshot(matchId) {
  const match = await prisma.match.findUnique({
    where: { id: matchId },
    include: {
      server: { select: { name: true, online: true, lastSeen: true } },
      teams: true,
      players: {
        include: {
          team: { select: { id: true, name: true, color: true } },
          user: { select: { username: true } }
        }
      }
    }
  });
  if (!match) return null;
  const leaderboard = match.players.map((p) => ({
    id: p.id,
    nfcToken: p.nfcToken,
    name: p.user?.username || `nfc:${p.nfcToken.slice(0, 8)}`,
    teamName: p.team?.name || null,
    teamColor: p.team?.color || '#94a3b8',
    hits: p.hits,
    deaths: p.deaths,
    shotsFired: p.shotsFired,
    points: p.points,
    kd: p.deaths === 0 ? p.hits : Number((p.hits / p.deaths).toFixed(2)),
    accuracy: p.shotsFired === 0 ? null : Number(((p.hits / p.shotsFired) * 100).toFixed(1))
  })).sort((a, b) => b.points - a.points);
  const feed = await prisma.hitEvent.findMany({
    where: { matchId },
    orderBy: { ts: 'desc' },
    take: 50
  });
  const now = Date.now();
  let remainingSeconds = null;
  if (match.status === 'active' && match.durationSeconds) {
    const elapsed = Math.floor((now - match.startedAt.getTime()) / 1000);
    remainingSeconds = Math.max(0, match.durationSeconds - elapsed);
  }
  return {
    match: {
      id: match.id, mode: match.mode, status: match.status,
      startedAt: match.startedAt, endedAt: match.endedAt,
      durationSeconds: match.durationSeconds, remainingSeconds
    },
    server: { name: match.server.name, online: match.server.online, lastSeen: match.server.lastSeen },
    teams: match.teams.map((t) => ({ id: t.id, name: t.name, color: t.color, totalPoints: t.totalPoints })),
    leaderboard,
    feed: feed.map((h) => ({ id: h.id, ts: h.ts, shooter: h.shooterNfc, target: h.targetNfc, points: h.points }))
  };
}

matchWss.on('connection', async (ws, info) => {
  const matchId = info.id;
  ws.matchId = matchId;
  addToSet(matchSubs, matchId, ws);
  const snap = await buildMatchSnapshot(matchId);
  if (snap) ws.send(JSON.stringify({ type: 'snapshot', ...snap }));
  ws.on('close', () => removeFromAllSets(matchSubs, ws));
  ws.on('message', async (raw) => {
    // Browser sends nothing relevant for match — could be ping/pong later
    try { JSON.parse(raw.toString()); } catch {}
  });
});

// Auto-push refreshed snapshots periodically (timer countdown updates).
setInterval(async () => {
  for (const [matchId, set] of matchSubs.entries()) {
    if (set.size === 0) continue;
    const snap = await buildMatchSnapshot(matchId);
    if (snap) broadcastSet(matchSubs, matchId, { type: 'snapshot', ...snap });
  }
}, 1000); // 1 s — only when there are subscribers

// --- Browser-ESP WS --------------------------------------------------------
const espWss = new WebSocketServer({ noServer: true });

async function buildEspSnapshot(espId) {
  const server = await prisma.espServer.findUnique({
    where: { id: espId },
    select: {
      id: true, name: true, online: true, lastSeen: true,
      mode: true, lobbySeconds: true, matchSeconds: true,
      zone1Points: true, zone2Points: true, zone3Points: true,
      teams: true, knownPlayers: true, startRequested: true
    }
  });
  if (!server) return null;
  return {
    type: 'snapshot',
    id: server.id,
    name: server.name,
    online: server.online,
    lastSeen: server.lastSeen,
    mode: server.mode,
    lobbySeconds: server.lobbySeconds,
    matchSeconds: server.matchSeconds,
    zone1Points: server.zone1Points,
    zone2Points: server.zone2Points,
    zone3Points: server.zone3Points,
    teams: server.teams || [],
    knownPlayers: server.knownPlayers || [],
    startRequested: server.startRequested
  };
}

espWss.on('connection', async (ws, info) => {
  const espId = info.id;
  ws.espId = espId;
  addToSet(espSubs, espId, ws);
  const snap = await buildEspSnapshot(espId);
  if (snap) ws.send(JSON.stringify(snap));
  ws.on('close', () => removeFromAllSets(espSubs, ws));
  ws.on('message', async (raw) => {
    let msg;
    try { msg = JSON.parse(raw.toString()); } catch { return; }

    if (msg.action === 'updateSettings') {
      // PIN-gated settings edit via WS
      const { pin, mode, lobbySeconds, matchSeconds, zone1Points, zone2Points, zone3Points, teams } = msg;
      const s = await prisma.espServer.findUnique({ where: { id: espId } });
      if (!s || s.pin !== String(pin || '')) {
        ws.send(JSON.stringify({ type: 'error', action: 'updateSettings', error: 'invalid_pin' }));
        return;
      }
      await prisma.espServer.update({
        where: { id: espId },
        data: {
          mode: mode ?? s.mode,
          lobbySeconds: lobbySeconds ?? s.lobbySeconds,
          matchSeconds: matchSeconds ?? s.matchSeconds,
          hitPoints: zone1Points ?? s.hitPoints,
          zone1Points: zone1Points ?? s.zone1Points,
          zone2Points: zone2Points ?? s.zone2Points,
          zone3Points: zone3Points ?? s.zone3Points,
          ...(teams !== undefined ? { teams } : {})
        }
      });
      const snap = await buildEspSnapshot(espId);
      if (snap) broadcastSet(espSubs, espId, snap);
      ws.send(JSON.stringify({ type: 'ok', action: 'updateSettings' }));
      return;
    }

    if (msg.action === 'startMatch') {
      const s = await prisma.espServer.findUnique({ where: { id: espId } });
      if (!s || s.pin !== String(msg.pin || '')) {
        ws.send(JSON.stringify({ type: 'error', action: 'startMatch', error: 'invalid_pin' }));
        return;
      }
      await prisma.espServer.update({ where: { id: espId }, data: { startRequested: true } });
      const snap = await buildEspSnapshot(espId);
      if (snap) broadcastSet(espSubs, espId, snap);
      ws.send(JSON.stringify({ type: 'ok', action: 'startMatch' }));
      return;
    }
  });
});

// --- HTTP-Upgrade Routing --------------------------------------------------
httpServer.on('upgrade', async (req, socket, head) => {
  const { pathname, query } = parse(req.url, true);

  // 1) ESP-Bridge WS
  if (pathname === '/api/bridge/ws') {
    let token = null;
    const authHeader = req.headers['authorization'];
    if (authHeader && authHeader.startsWith('Bearer ')) token = authHeader.slice(7).trim();
    if (!token && typeof query.token === 'string') token = query.token;
    if (!token) {
      socket.write('HTTP/1.1 401 Unauthorized\r\n\r\n');
      socket.destroy();
      return;
    }
    let server;
    try {
      server = await prisma.espServer.findUnique({ where: { pin: token }, select: { id: true, name: true } });
    } catch (e) {
      console.error('ws auth db error', e?.message);
    }
    if (!server) {
      socket.write('HTTP/1.1 401 Unauthorized\r\n\r\n');
      socket.destroy();
      return;
    }
    await prisma.espServer.update({ where: { id: server.id }, data: { lastSeen: new Date(), online: true } });
    bridgeWss.handleUpgrade(req, socket, head, (ws) => bridgeWss.emit('connection', ws, server));
    return;
  }

  // 2) Browser-Match WS
  const matchMatch = pathname?.match(/^\/api\/ws\/match\/([a-zA-Z0-9_-]+)$/);
  if (matchMatch) {
    matchWss.handleUpgrade(req, socket, head, (ws) => matchWss.emit('connection', ws, { id: matchMatch[1] }));
    return;
  }

  // 3) Browser-ESP WS
  const espMatch = pathname?.match(/^\/api\/ws\/esp\/([a-zA-Z0-9_-]+)$/);
  if (espMatch) {
    espWss.handleUpgrade(req, socket, head, (ws) => espWss.emit('connection', ws, { id: espMatch[1] }));
    return;
  }

  socket.destroy();
});

httpServer.listen(port, hostname, () => {
  console.log(`hmyLaser32 ready on http://${hostname}:${port}`);
  console.log(`  WS bridge:  /api/bridge/ws`);
  console.log(`  WS match:   /api/ws/match/<id>`);
  console.log(`  WS esp:     /api/ws/esp/<id>`);
});
