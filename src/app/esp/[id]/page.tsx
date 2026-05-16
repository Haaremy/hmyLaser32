import { notFound } from 'next/navigation';
import { db } from '@/lib/db';
import { isOnline } from '@/lib/bridge';
import { parseTeams } from '@/lib/teams';
import { EspDetail } from './EspDetail';

export const dynamic = 'force-dynamic';

export default async function EspDetailPage({ params }: { params: { id: string } }) {
  const server = await db.espServer.findUnique({
    where: { id: params.id },
    include: {
      matches: {
        orderBy: { startedAt: 'desc' },
        take: 10,
        select: { id: true, startedAt: true, status: true, mode: true, durationSeconds: true }
      }
    }
  });
  if (!server) notFound();

  return (
    <EspDetail
      id={server.id}
      name={server.name}
      online={isOnline(server.lastSeen)}
      lastSeen={server.lastSeen?.toISOString() ?? null}
      mode={server.mode}
      lobbySeconds={server.lobbySeconds}
      matchSeconds={server.matchSeconds}
      teams={parseTeams(server.teams)}
      startRequested={server.startRequested}
      matches={server.matches.map((m) => ({
        id: m.id,
        startedAt: m.startedAt.toISOString(),
        status: m.status,
        mode: m.mode,
        durationSeconds: m.durationSeconds
      }))}
    />
  );
}
