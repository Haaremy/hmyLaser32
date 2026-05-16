import { NextResponse } from 'next/server';
import { db } from '@/lib/db';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

// GET, vom ESP-Server selbst aufgerufen (Bearer = PIN).
// Liefert Match-Defaults + startRequested-Flag. Konsumiert das Flag (setzt es
// zurück), damit der ESP nicht wiederholt startet.
export async function GET(req: Request) {
  const auth = req.headers.get('authorization');
  if (!auth || !auth.startsWith('Bearer ')) {
    return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  }
  const pin = auth.substring(7).trim();
  const server = await db.espServer.findUnique({
    where: { pin },
    select: { id: true, mode: true, lobbySeconds: true, matchSeconds: true, startRequested: true }
  });
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });

  // Flag konsumieren: wenn true, sofort zurücksetzen (damit ESP nur einmal startet)
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
    startRequested: server.startRequested
  });
}
