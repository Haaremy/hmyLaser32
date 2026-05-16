import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { isOnline } from '@/lib/bridge';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

const RETENTION_MS = 24 * 60 * 60 * 1000; // 24 h

/** Liste aller bekannten ESP-Server für die Startseite.
 *  Räumt zugleich alte (>24 h offline) Einträge aus der DB.
 */
export async function GET() {
  const cutoff = new Date(Date.now() - RETENTION_MS);
  // Stale ESPs entfernen (lastSeen NULL counts as never-seen → 24h nach createdAt)
  await db.espServer.deleteMany({
    where: {
      OR: [
        { lastSeen: { lt: cutoff } },
        { AND: [{ lastSeen: null }, { createdAt: { lt: cutoff } }] }
      ]
    }
  });

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
