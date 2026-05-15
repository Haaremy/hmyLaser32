import { db } from './db';

export async function authenticateBridge(authHeader: string | null): Promise<{ id: string; name: string } | null> {
  if (!authHeader || !authHeader.startsWith('Bearer ')) return null;
  const code = authHeader.substring(7).trim();
  if (!code) return null;
  const server = await db.espServer.findUnique({
    where: { accessCode: code },
    select: { id: true, name: true }
  });
  if (!server) return null;
  await db.espServer.update({
    where: { id: server.id },
    data: { lastSeen: new Date() }
  });
  return server;
}

export function generateAccessCode(): string {
  const bytes = new Uint8Array(24);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('');
}
