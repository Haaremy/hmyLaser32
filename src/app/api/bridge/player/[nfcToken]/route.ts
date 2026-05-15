import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { authenticateBridge } from '@/lib/bridge';

export const runtime = 'nodejs';

export async function GET(req: Request, { params }: { params: { nfcToken: string } }) {
  const server = await authenticateBridge(req.headers.get('authorization'));
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });

  const user = await db.user.findUnique({
    where: { nfcToken: params.nfcToken },
    select: { id: true, username: true, role: true }
  });
  if (!user) return NextResponse.json({ error: 'not_found' }, { status: 404 });

  const lastTeams = await db.matchPlayer.findMany({
    where: { userId: user.id, teamId: { not: null } },
    include: { team: { select: { name: true, color: true } } },
    orderBy: { match: { startedAt: 'desc' } },
    take: 5
  });

  return NextResponse.json({
    user: { username: user.username, role: user.role },
    recentTeams: lastTeams.map((p) => p.team?.name).filter(Boolean)
  });
}
