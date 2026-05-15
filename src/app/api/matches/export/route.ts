import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

export async function GET(req: Request) {
  const session = await getSession();
  if (!session.userId) return NextResponse.json({ error: 'unauthenticated' }, { status: 401 });
  const url = new URL(req.url);
  const format = url.searchParams.get('format') === 'csv' ? 'csv' : 'json';

  const players = await db.matchPlayer.findMany({
    include: {
      match: { select: { id: true, mode: true, startedAt: true, endedAt: true } },
      team: { select: { name: true, color: true } },
      user: { select: { username: true } }
    },
    orderBy: { match: { startedAt: 'desc' } },
    take: 5000
  });

  if (format === 'csv') {
    const header = 'match_id,started_at,mode,player,team,hits,deaths,points\n';
    const rows = players
      .map((p) =>
        [
          p.match.id,
          p.match.startedAt.toISOString(),
          p.match.mode || '',
          p.user?.username || `nfc:${p.nfcToken}`,
          p.team?.name || '',
          p.hits,
          p.deaths,
          p.points
        ]
          .map((v) => (typeof v === 'string' && v.includes(',') ? `"${v}"` : v))
          .join(',')
      )
      .join('\n');
    return new NextResponse(header + rows + '\n', {
      headers: {
        'Content-Type': 'text/csv; charset=utf-8',
        'Content-Disposition': 'attachment; filename="hmylaser32-matches.csv"'
      }
    });
  }

  return NextResponse.json({ players });
}
