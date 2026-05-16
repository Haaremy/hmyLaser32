import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { parseTeams } from '@/lib/teams';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

// GET, vom ESP-Server selbst aufgerufen (Bearer = PIN).
// Liefert Match-Defaults + Teams + startRequested-Flag (atomisch konsumiert).
export async function GET(req: Request) {
  const auth = req.headers.get('authorization');
  if (!auth || !auth.startsWith('Bearer ')) {
    return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  }
  const pin = auth.substring(7).trim();
  const server = await db.espServer.findUnique({
    where: { pin },
    select: {
      id: true, mode: true, lobbySeconds: true, matchSeconds: true,
      teams: true, startRequested: true
    }
  });
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });

  if (server.startRequested) {
    await db.espServer.update({
      where: { id: server.id },
      data: { startRequested: false, lastSeen: new Date(), online: true }
    });
  } else {
    await db.espServer.update({
      where: { id: server.id },
      data: { lastSeen: new Date(), online: true }
    });
  }

  return NextResponse.json({
    mode: server.mode,
    lobbySeconds: server.lobbySeconds,
    matchSeconds: server.matchSeconds,
    teams: parseTeams(server.teams),
    startRequested: server.startRequested
  });
}
