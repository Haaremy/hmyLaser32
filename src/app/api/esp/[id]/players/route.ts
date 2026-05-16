import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { parseKnownPlayers } from '@/lib/players';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

// GET — bekannte Clients eines ESP-Servers (öffentlich, read-only).
export async function GET(_req: Request, { params }: { params: { id: string } }) {
  const server = await db.espServer.findUnique({
    where: { id: params.id },
    select: { knownPlayers: true }
  });
  if (!server) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  return NextResponse.json({ players: parseKnownPlayers(server.knownPlayers) });
}
