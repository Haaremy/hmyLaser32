import { NextResponse } from 'next/server';
import { z } from 'zod';
import { db } from '@/lib/db';
import { authenticateBridge } from '@/lib/bridge';

export const runtime = 'nodejs';

const schema = z.object({
  matchId: z.string().min(1),
  players: z
    .array(
      z.object({
        nfcToken: z.string().min(1).max(64),
        teamName: z.string().max(32).optional(),
        hits: z.number().int().min(0).default(0),
        deaths: z.number().int().min(0).default(0),
        points: z.number().int().default(0),
        shotsFired: z.number().int().min(0).optional().default(0)
      })
    )
    .max(64)
});

export async function POST(req: Request) {
  const server = await authenticateBridge(req.headers.get('authorization'));
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });

  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }

  const match = await db.match.findUnique({
    where: { id: body.matchId },
    include: { teams: true }
  });
  if (!match || match.serverId !== server.id) {
    return NextResponse.json({ error: 'match_not_found' }, { status: 404 });
  }

  const teamsByName = new Map(match.teams.map((t) => [t.name, t]));
  const usersByToken = await db.user.findMany({
    where: { nfcToken: { in: body.players.map((p) => p.nfcToken) } },
    select: { id: true, nfcToken: true }
  });
  const userIdByToken = new Map(usersByToken.map((u) => [u.nfcToken, u.id]));

  const teamTotals = new Map<string, number>();
  for (const p of body.players) {
    const teamId = p.teamName ? teamsByName.get(p.teamName)?.id : undefined;
    if (teamId) teamTotals.set(teamId, (teamTotals.get(teamId) || 0) + p.points);
    await db.matchPlayer.create({
      data: {
        matchId: match.id,
        userId: userIdByToken.get(p.nfcToken) || null,
        nfcToken: p.nfcToken,
        teamId: teamId || null,
        hits: p.hits,
        deaths: p.deaths,
        points: p.points,
        shotsFired: p.shotsFired
      }
    });
  }
  await Promise.all(
    Array.from(teamTotals.entries()).map(([teamId, total]) =>
      db.team.update({ where: { id: teamId }, data: { totalPoints: total } })
    )
  );

  const now = new Date();
  await db.match.update({
    where: { id: match.id },
    data: {
      endedAt: now,
      status: 'finished',
      durationSeconds: match.durationSeconds ?? Math.round((now.getTime() - match.startedAt.getTime()) / 1000)
    }
  });

  return NextResponse.json({ ok: true });
}
