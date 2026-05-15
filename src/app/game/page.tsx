import { redirect } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';

export const dynamic = 'force-dynamic';

export default async function GamePage() {
  const session = await getSession();
  if (!session.userId) redirect('/auth/login');
  const t = await getTranslations('game');

  const matches = await db.match.findMany({
    orderBy: { startedAt: 'desc' },
    take: 50,
    include: {
      server: { select: { name: true } },
      _count: { select: { players: true } }
    }
  });

  return (
    <>
      <h1>{t('title')}</h1>
      <p>
        <a className="btn" href="/api/matches/export?format=csv">{t('export_csv')}</a>{' '}
        <a className="btn" href="/api/matches/export?format=json">{t('export_json')}</a>
      </p>
      <table>
        <thead>
          <tr><th>ID</th><th>Server</th><th>Mode</th><th>Status</th><th>Started</th><th>Players</th></tr>
        </thead>
        <tbody>
          {matches.map((m) => (
            <tr key={m.id}>
              <td><a href={`/matches/${m.id}`}><code className="code">{m.id.slice(0, 8)}</code></a></td>
              <td>{m.server.name}</td>
              <td>{m.mode || '—'}</td>
              <td>{m.status}</td>
              <td>{new Date(m.startedAt).toLocaleString()}</td>
              <td>{m._count.players}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </>
  );
}
