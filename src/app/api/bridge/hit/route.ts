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
  ts: z.number().int().optional()
});

/**
 * Persistiert Hit-Events. Quelle für den Live-Feed in der Match-Detail-Ansicht.
 * ESPs OHNE Hit-Detail-Stream können statt dessen den Score-Diff in match/end senden;
 * der Live-Feed-Renderer fällt dann auf score-basierte Events zurück.
 */
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
  return NextResponse.json({ ok: true, id: hit.id, ts: hit.ts });
}
