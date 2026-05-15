import { notFound } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';

export const dynamic = 'force-dynamic';

export default async function MatchDetailPage({ params }: { params: { id: string } }) {
  const t = await getTranslations('match');
  const match = await db.match.findUnique({
    where: { id: params.id },
    include: {
      server: { select: { name: true } },
      teams: { include: { players: { include: { user: { select: { username: true } } } } } }
    }
  });
  if (!match) notFound();

  return (
    <>
      <h1>{t('title')}: {match.mode || params.id.slice(0, 8)}</h1>
      <div className="card">
        <div className="grid grid-2">
          <div><strong>{t('started')}:</strong> {new Date(match.startedAt).toLocaleString()}</div>
          <div><strong>{t('ended')}:</strong> {match.endedAt ? new Date(match.endedAt).toLocaleString() : '—'}</div>
          <div><strong>{t('duration')}:</strong> {match.durationSeconds ? `${match.durationSeconds}s` : '—'}</div>
          <div><strong>{t('status')}:</strong> {match.status}</div>
          <div><strong>Server:</strong> {match.server.name}</div>
        </div>
      </div>

      <h2>{t('teams')}</h2>
      {match.teams.map((team) => (
        <div key={team.id} className="card" style={{ borderLeft: `4px solid ${team.color}` }}>
          <h3 style={{ marginTop: 0, color: team.color }}>{team.name} — {team.totalPoints} pts</h3>
          <table>
            <thead>
              <tr><th>{t('players')}</th><th>Hits</th><th>Deaths</th><th>Points</th></tr>
            </thead>
            <tbody>
              {team.players.map((p) => (
                <tr key={p.id}>
                  <td>{p.user?.username || `(${p.nfcToken.slice(0, 8)}…)`}</td>
                  <td>{p.hits}</td>
                  <td>{p.deaths}</td>
                  <td>{p.points}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      ))}
    </>
  );
}
