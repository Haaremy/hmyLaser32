import { db } from './db';

/**
 * Authentication für die ESP→Server Bridge.
 * Der ESP sendet seinen PIN als `Authorization: Bearer <pin>` Header.
 */
export async function authenticateBridge(authHeader: string | null): Promise<{ id: string; name: string } | null> {
  if (!authHeader || !authHeader.startsWith('Bearer ')) return null;
  const pin = authHeader.substring(7).trim();
  if (!pin) return null;
  const server = await db.espServer.findUnique({
    where: { pin },
    select: { id: true, name: true }
  });
  if (!server) return null;
  await db.espServer.update({
    where: { id: server.id },
    data: { lastSeen: new Date(), online: true }
  });
  return server;
}

/** Same as authenticateBridge but with explicit pin string (e.g. from WebSocket query param). */
export async function authenticateByPin(pin: string | null | undefined): Promise<{ id: string; name: string } | null> {
  if (!pin) return null;
  const server = await db.espServer.findUnique({
    where: { pin: pin.trim() },
    select: { id: true, name: true }
  });
  if (!server) return null;
  await db.espServer.update({
    where: { id: server.id },
    data: { lastSeen: new Date(), online: true }
  });
  return server;
}

/** Used by `/api/match/[id]/settings` when the user types a PIN to unlock the settings tab. */
export async function pinMatchesServer(serverId: string, pin: string): Promise<boolean> {
  const server = await db.espServer.findUnique({
    where: { id: serverId },
    select: { pin: true }
  });
  if (!server) return false;
  return server.pin === pin.trim();
}

export function isOnline(lastSeen: Date | null | undefined, thresholdSeconds = 90): boolean {
  if (!lastSeen) return false;
  return Date.now() - lastSeen.getTime() < thresholdSeconds * 1000;
}
