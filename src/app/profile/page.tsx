import { redirect } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';

export const dynamic = 'force-dynamic';

export default async function ProfilePage() {
  const session = await getSession();
  if (!session.userId) redirect('/auth/login');

  const t = await getTranslations('profile');

  const user = await db.user.findUnique({
    where: { id: session.userId },
    select: { id: true, username: true, role: true, nfcToken: true, createdAt: true }
  });
  if (!user) redirect('/auth/login');

  const players = await db.matchPlayer.findMany({
    where: { userId: user.id },
    include: { match: { select: { id: true, startedAt: true, endedAt: true, mode: true, status: true } } },
    orderBy: { match: { startedAt: 'desc' } },
    take: 20
  });

  const totals = players.reduce(
    (acc, p) => ({
      hits: acc.hits + p.hits,
      deaths: acc.deaths + p.deaths,
      points: acc.points + p.points
    }),
    { hits: 0, deaths: 0, points: 0 }
  );
  const kd = totals.deaths === 0 ? totals.hits : (totals.hits / totals.deaths).toFixed(2);

  return (
    <>
      <h1>{t('title')}</h1>

      <div className="card">
        <div className="grid grid-2">
          <div><strong>{t('username')}:</strong> {user.username}</div>
          <div><strong>{t('role')}:</strong> {user.role}</div>
        </div>
        <h3>{t('nfc_token')}</h3>
        <code className="code" style={{ wordBreak: 'break-all', fontSize: '1rem' }}>{user.nfcToken}</code>
        <p style={{ marginTop: '0.5rem' }}>{t('nfc_hint')}</p>
      </div>

      <div className="card">
        <h2 style={{ marginTop: 0 }}>{t('stats')}</h2>
        <div className="grid grid-3">
          <div><strong>{t('matches_played')}:</strong> {players.length}</div>
          <div><strong>{t('total_hits')}:</strong> {totals.hits}</div>
          <div><strong>{t('total_deaths')}:</strong> {totals.deaths}</div>
          <div><strong>{t('total_points')}:</strong> {totals.points}</div>
          <div><strong>{t('kd_ratio')}:</strong> {kd}</div>
        </div>
      </div>

      <h2>{t('match_history')}</h2>
      {players.length === 0 ? (
        <p>{t('no_matches')}</p>
      ) : (
        <table>
          <thead>
            <tr><th>Match</th><th>Mode</th><th>Hits</th><th>Deaths</th><th>Points</th></tr>
          </thead>
          <tbody>
            {players.map((p) => (
              <tr key={p.id}>
                <td><a href={`/matches/${p.match.id}`}>{new Date(p.match.startedAt).toLocaleString()}</a></td>
                <td>{p.match.mode || '—'}</td>
                <td>{p.hits}</td>
                <td>{p.deaths}</td>
                <td>{p.points}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </>
  );
}
