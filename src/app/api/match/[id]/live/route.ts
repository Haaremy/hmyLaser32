import { NextResponse } from 'next/server';
import { db } from '@/lib/db';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

/** Live-State eines Matches für die Detail-Ansicht (polling alle ~2s). */
export async function GET(req: Request, { params }: { params: { id: string } }) {
  const url = new URL(req.url);
  const since = url.searchParams.get('since'); // ISO-Timestamp

  const match = await db.match.findUnique({
    where: { id: params.id },
    include: {
      server: { select: { name: true, online: true, lastSeen: true } },
      teams: true,
      players: {
        include: {
          team: { select: { id: true, name: true, color: true } },
          user: { select: { username: true } }
        }
      }
    }
  });
  if (!match) return NextResponse.json({ error: 'not_found' }, { status: 404 });

  // Players um K/D + Trefferquote anreichern
  const playersScored = match.players
    .map((p) => ({
      id: p.id,
      nfcToken: p.nfcToken,
      name: p.user?.username || `nfc:${p.nfcToken.slice(0, 8)}`,
      teamName: p.team?.name || null,
      teamColor: p.team?.color || '#94a3b8',
      hits: p.hits,
      deaths: p.deaths,
      shotsFired: p.shotsFired,
      points: p.points,
      kd: p.deaths === 0 ? p.hits : Number((p.hits / p.deaths).toFixed(2)),
      accuracy: p.shotsFired === 0 ? null : Number(((p.hits / p.shotsFired) * 100).toFixed(1))
    }))
    .sort((a, b) => b.points - a.points);

  // Live-Feed: Hit-Events, optional ab `since`
  const feed = await db.hitEvent.findMany({
    where: {
      matchId: match.id,
      ...(since ? { ts: { gt: new Date(since) } } : {})
    },
    orderBy: { ts: 'desc' },
    take: 50
  });

  // Zeit-Restbestimmung
  const now = Date.now();
  let remainingSeconds: number | null = null;
  if (match.status === 'active' && match.durationSeconds) {
    const elapsed = Math.floor((now - match.startedAt.getTime()) / 1000);
    remainingSeconds = Math.max(0, match.durationSeconds - elapsed);
  }

  return NextResponse.json({
    match: {
      id: match.id,
      mode: match.mode,
      status: match.status,
      startedAt: match.startedAt,
      endedAt: match.endedAt,
      durationSeconds: match.durationSeconds,
      remainingSeconds
    },
    server: {
      name: match.server.name,
      online: match.server.online,
      lastSeen: match.server.lastSeen
    },
    teams: match.teams.map((t) => ({ id: t.id, name: t.name, color: t.color, totalPoints: t.totalPoints })),
    leaderboard: playersScored,
    feed: feed.map((h) => ({
      id: h.id,
      ts: h.ts,
      shooter: h.shooterNfc,
      target: h.targetNfc,
      points: h.points
    }))
  });
}
