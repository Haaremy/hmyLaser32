import { NextResponse } from 'next/server';
import { Prisma } from '@prisma/client';
import { db } from '@/lib/db';
import { knownPlayersSchema } from '@/lib/players';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

export async function POST(req: Request) {
  const auth = req.headers.get('authorization');
  if (!auth || !auth.startsWith('Bearer ')) {
    return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  }
  const pin = auth.substring(7).trim();
  const server = await db.espServer.findUnique({
    where: { pin },
    select: { id: true, matches: { where: { status: 'active' }, orderBy: { startedAt: 'desc' }, take: 1, select: { id: true } } }
  });
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });

  let body: unknown;
  try {
    body = await req.json();
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const parsed = knownPlayersSchema.safeParse((body as any)?.players);
  if (!parsed.success) return NextResponse.json({ error: 'invalid_body' }, { status: 400 });

  await db.espServer.update({
    where: { id: server.id },
    data: {
      knownPlayers: parsed.data as unknown as Prisma.InputJsonValue,
      lastSeen: new Date(),
      online: true
    }
  });
  const activeMatch = server.matches[0];
  if (activeMatch) {
    for (const p of parsed.data) {
      const nfcToken = p.nfcToken || `nec:${p.playerId.toString(16).padStart(8, '0')}`;
      await db.matchPlayer.upsert({
        where: { matchId_nfcToken: { matchId: activeMatch.id, nfcToken } },
        update: {
          points: p.points,
          shotsFired: p.shots,
          deaths: p.rxHits
        },
        create: {
          matchId: activeMatch.id,
          nfcToken,
          points: p.points,
          shotsFired: p.shots,
          deaths: p.rxHits,
          hits: 0
        }
      });
    }
    (globalThis as any).wsHub?.broadcastMatch?.(activeMatch.id, { type: 'snapshot' });
  }
  (globalThis as any).wsHub?.broadcastEsp?.(server.id, { type: 'invalidate' });
  return NextResponse.json({ ok: true, count: parsed.data.length });
}
