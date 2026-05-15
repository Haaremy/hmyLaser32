// Custom Next.js entrypoint adding a WebSocket endpoint at /api/bridge/ws.
// Authentication: ?token=<accessCode> query parameter (ESP32 sends the same access code as the REST bridge).
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

const wss = new WebSocketServer({ noServer: true });

const espRooms = new Map(); // matchId → Set<WebSocket>

function broadcast(matchId, payload) {
  const room = espRooms.get(matchId);
  if (!room) return;
  const data = JSON.stringify(payload);
  for (const ws of room) {
    if (ws.readyState === 1) ws.send(data);
  }
}

httpServer.on('upgrade', async (req, socket, head) => {
  const { pathname, query } = parse(req.url, true);
  if (pathname !== '/api/bridge/ws') {
    socket.destroy();
    return;
  }

  // Bearer via header (Authorization) or via ?token=
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
    server = await prisma.espServer.findUnique({ where: { accessCode: token }, select: { id: true } });
  } catch (e) {
    console.error('ws auth db error', e);
  }
  if (!server) {
    socket.write('HTTP/1.1 401 Unauthorized\r\n\r\n');
    socket.destroy();
    return;
  }

  wss.handleUpgrade(req, socket, head, (ws) => {
    ws.serverId = server.id;
    ws.matchId = null;
    ws.on('message', (raw) => {
      let msg;
      try {
        msg = JSON.parse(raw.toString());
      } catch {
        return;
      }
      if (msg.type === 'subscribe' && typeof msg.matchId === 'string') {
        ws.matchId = msg.matchId;
        if (!espRooms.has(ws.matchId)) espRooms.set(ws.matchId, new Set());
        espRooms.get(ws.matchId).add(ws);
        ws.send(JSON.stringify({ type: 'subscribed', matchId: ws.matchId }));
        return;
      }
      if (msg.type === 'hit_event' && typeof msg.matchId === 'string') {
        broadcast(msg.matchId, { type: 'hit_event', ...msg });
      }
      if (msg.type === 'timer_sync' && typeof msg.matchId === 'string') {
        broadcast(msg.matchId, { type: 'timer_sync', ...msg });
      }
      if (msg.type === 'game_state_update' && typeof msg.matchId === 'string') {
        broadcast(msg.matchId, { type: 'game_state_update', ...msg });
      }
    });
    ws.on('close', () => {
      if (ws.matchId && espRooms.has(ws.matchId)) espRooms.get(ws.matchId).delete(ws);
    });
    ws.send(JSON.stringify({ type: 'hello', ts: Date.now() }));
  });
});

httpServer.listen(port, hostname, () => {
  console.log(`hmyLaser32 ready on http://${hostname}:${port}  (WS: /api/bridge/ws)`);
});
