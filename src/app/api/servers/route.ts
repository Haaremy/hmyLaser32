import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { isOnline } from '@/lib/bridge';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

/** Liste aller bekannten ESP-Server für die Startseite. */
export async function GET() {
  const servers = await db.espServer.findMany({
    orderBy: [{ online: 'desc' }, { lastSeen: 'desc' }],
    select: {
      id: true,
      name: true,
      lastSeen: true,
      online: true,
      matches: {
        where: { status: 'active' },
        orderBy: { startedAt: 'desc' },
        take: 1,
        select: { id: true, mode: true, startedAt: true, durationSeconds: true }
      }
    }
  });
  return NextResponse.json({
    servers: servers.map((s) => ({
      id: s.id,
      name: s.name,
      online: isOnline(s.lastSeen),
      lastSeen: s.lastSeen,
      activeMatch: s.matches[0] || null
    }))
  });
}
