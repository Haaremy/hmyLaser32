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
  const tc = await getTranslations('common');
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
      <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', fontFamily: 'var(--hmy-font-family-mono)' }}>
        {server.name}
      </h1>
      <p>
        <span className={`hmy-lt-pill ${online ? 'hmy-lt-pill--success' : 'hmy-lt-pill--muted'}`}>
          {online ? tc('online') : tc('offline')}
        </span>{' '}
        <span style={{ color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
          · <RelativeTime ts={server.lastSeen ? new Date(server.lastSeen).toISOString() : null} />
        </span>
      </p>

      {active && (
        <div className="hmy-card">
          <div className="hmy-card__header">{t('active_match')}</div>
          <div className="hmy-card__body">
            <Link href={`/match/${active.id}`} className="hmy-btn hmy-btn--primary">{tm('title')} →</Link>
          </div>
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
              <th className="num">{tm('duration')}</th>
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
