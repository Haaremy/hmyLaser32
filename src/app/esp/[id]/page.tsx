import Link from 'next/link';
import { notFound } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { isOnline } from '@/lib/bridge';
import { RelativeTime } from '@/components/RelativeTime';

export const dynamic = 'force-dynamic';

export default async function EspDetailPage({ params }: { params: { id: string } }) {
  const t = await getTranslations('home');
  const tm = await getTranslations('match');
  const server = await db.espServer.findUnique({
    where: { id: params.id },
    include: {
      matches: { orderBy: { startedAt: 'desc' }, take: 10 }
    }
  });
  if (!server) notFound();

  const online = isOnline(server.lastSeen);
  const active = server.matches.find((m) => m.status === 'active');

  return (
    <>
      <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', fontFamily: 'var(--font-family-mono)' }}>
        {server.name}
      </h1>
      <p style={{ color: online ? 'var(--color-status-success)' : 'var(--color-text-muted)' }}>
        ● {online ? 'Online' : 'Offline'} · <RelativeTime ts={server.lastSeen ? new Date(server.lastSeen).toISOString() : null} />
      </p>

      {active && (
        <div className="card">
          <strong>{t('active_match')}</strong>
          <p>
            <Link href={`/match/${active.id}`} className="btn btn-primary">
              {t('open_match')}
            </Link>
          </p>
        </div>
      )}

      <h2>{tm('title')} ({server.matches.length})</h2>
      {server.matches.length === 0 ? (
        <p>—</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>{tm('started')}</th>
              <th>{tm('mode')}</th>
              <th>{tm('status')}</th>
              <th>{tm('duration')}</th>
            </tr>
          </thead>
          <tbody>
            {server.matches.map((m) => (
              <tr key={m.id}>
                <td><Link href={`/match/${m.id}`}>{new Date(m.startedAt).toLocaleString()}</Link></td>
                <td>{m.mode || '—'}</td>
                <td>{m.status}</td>
                <td className="num">{m.durationSeconds ? `${m.durationSeconds}s` : '—'}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </>
  );
}
