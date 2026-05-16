import { NextResponse } from 'next/server';
import { z } from 'zod';
import { db } from '@/lib/db';
import { authenticateBridge } from '@/lib/bridge';

export const runtime = 'nodejs';

const schema = z.object({
  matchId: z.string().min(1),
  shooterNfc: z.string().max(64).optional(),
  targetNfc: z.string().max(64).optional(),
  points: z.number().int().default(0),
  ts: z.number().int().optional(),
  // v8: optional per-player detail stats for the shooter
  shooterHits: z.number().int().min(0).optional(),
  shooterDeaths: z.number().int().min(0).optional(),
  shooterShots: z.number().int().min(0).optional()
});

export async function POST(req: Request) {
  const server = await authenticateBridge(req.headers.get('authorization'));
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  let data: z.infer<typeof schema>;
  try {
    data = schema.parse(await req.json());
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const match = await db.match.findUnique({ where: { id: data.matchId }, select: { id: true, serverId: true } });
  if (!match || match.serverId !== server.id) {
    return NextResponse.json({ error: 'match_not_found' }, { status: 404 });
  }
  const hit = await db.hitEvent.create({
    data: {
      matchId: data.matchId,
      ts: data.ts ? new Date(data.ts) : new Date(),
      shooterNfc: data.shooterNfc || null,
      targetNfc: data.targetNfc || null,
      points: data.points
    }
  });

  // v8: aggregate stats on the MatchPlayer for the shooter (upsert by nfcToken)
  if (data.shooterNfc) {
    try {
      await db.matchPlayer.upsert({
        where: { matchId_nfcToken: { matchId: data.matchId, nfcToken: data.shooterNfc } },
        update: {
          ...(data.shooterHits  !== undefined ? { hits: data.shooterHits   } : {}),
          ...(data.shooterDeaths!== undefined ? { deaths: data.shooterDeaths } : {}),
          ...(data.shooterShots !== undefined ? { shotsFired: data.shooterShots } : {}),
          points: { increment: data.points }
        },
        create: {
          matchId: data.matchId,
          nfcToken: data.shooterNfc,
          hits: data.shooterHits ?? 0,
          deaths: data.shooterDeaths ?? 0,
          shotsFired: data.shooterShots ?? 0,
          points: data.points
        }
      });
    } catch (e) {
      console.error('upsert shooter stats', (e as Error).message);
    }
  }

  // Broadcast to subscribed browsers
  (globalThis as any).wsHub?.broadcastMatch?.(data.matchId, { type: 'hit' });

  return NextResponse.json({ ok: true, id: hit.id, ts: hit.ts });
}
