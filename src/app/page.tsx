import Link from 'next/link';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { isOnline } from '@/lib/bridge';
import { RelativeTime } from '@/components/RelativeTime';

export const dynamic = 'force-dynamic';

export default async function HomePage() {
  const t = await getTranslations('home');
  const tc = await getTranslations('common');

  const servers = await db.espServer.findMany({
    orderBy: [{ online: 'desc' }, { lastSeen: 'desc' }],
    select: {
      id: true,
      name: true,
      lastSeen: true,
      online: true,
      matches: {
        where: { status: 'active' },
        orderBy: { startedAt: 'desc' },
        take: 1,
        select: { id: true, mode: true, startedAt: true, durationSeconds: true }
      }
    }
  });

  return (
    <>
      <section className="hmy-lt-hero">
        <h1>{t('title')}</h1>
        <p>{t('subtitle')}</p>
      </section>

      <h2>{t('esps_heading')}</h2>

      {servers.length === 0 ? (
        <div className="hmy-alert hmy-alert--info">{t('esps_empty')}</div>
      ) : (
        <div className="hmy-lt-esp-grid">
          {servers.map((s) => {
            const online = isOnline(s.lastSeen);
            const m = s.matches[0];
            const target = m ? `/match/${m.id}` : `/esp/${s.id}`;
            return (
              <Link href={target} key={s.id} className="hmy-lt-esp-card">
                <div className="hmy-lt-esp-card__name">{s.name}</div>
                <div className={`hmy-lt-esp-card__status ${online ? 'is-online' : 'is-offline'}`}>
                  {online ? tc('online') : tc('offline')}
                  <span style={{ marginLeft: 'auto', fontWeight: 400, textTransform: 'none', letterSpacing: 0 }}>
                    <RelativeTime ts={s.lastSeen ? new Date(s.lastSeen).toISOString() : null} />
                  </span>
                </div>
                <div className="hmy-lt-esp-card__match">
                  {m ? (
                    <>
                      <strong>{m.mode || 'Match'}</strong>
                      Läuft seit {new Date(m.startedAt).toLocaleTimeString()}
                    </>
                  ) : (
                    <em>{t('no_active_match')}</em>
                  )}
                </div>
              </Link>
            );
          })}
        </div>
      )}
    </>
  );
}
