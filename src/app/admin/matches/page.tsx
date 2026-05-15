import Link from 'next/link';
import { redirect } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';

export const dynamic = 'force-dynamic';

export default async function AdminMatchesPage() {
  const session = await getSession();
  if (!session.userId) redirect('/auth/login');
  const t = await getTranslations('admin');
  if (session.role !== 'ADMIN') return <div className="alert alert-error">{t('no_access')}</div>;

  const matches = await db.match.findMany({
    orderBy: { startedAt: 'desc' },
    take: 100,
    include: { server: { select: { name: true } }, _count: { select: { players: true } } }
  });

  return (
    <>
      <h1>{t('matches')}</h1>
      <table>
        <thead>
          <tr><th>ID</th><th>Server</th><th>Mode</th><th>Status</th><th>Started</th><th>Players</th></tr>
        </thead>
        <tbody>
          {matches.map((m) => (
            <tr key={m.id}>
              <td><Link href={`/matches/${m.id}`}><code className="code">{m.id.slice(0, 8)}</code></Link></td>
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
