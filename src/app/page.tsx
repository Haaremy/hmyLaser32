import Link from 'next/link';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { isOnline } from '@/lib/bridge';
import { RelativeTime } from '@/components/RelativeTime';

export const dynamic = 'force-dynamic';

export default async function HomePage() {
  const t = await getTranslations('home');

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
      <section className="hero">
        <h1>{t('title')}</h1>
        <p className="subtitle">{t('subtitle')}</p>

        <div className="quicklinks">
          <Link href="/wiki/build-guide" className="quicklink">🛠 {t('wiki_short')}</Link>
          <Link href="/profile" className="quicklink">👤 {t('profile_short')}</Link>
        </div>
      </section>

      <h2>{t('esps_heading')}</h2>

      {servers.length === 0 ? (
        <div className="alert alert-info">{t('esps_empty')}</div>
      ) : (
        <div className="esp-grid">
          {servers.map((s) => {
            const online = isOnline(s.lastSeen);
            const m = s.matches[0];
            const target = m ? `/match/${m.id}` : `/esp/${s.id}`;
            return (
              <Link href={target} key={s.id} className="esp-card">
                <div className="esp-name">{s.name}</div>
                <div className={`esp-status ${online ? 'online' : 'offline'}`}>
                  {online ? t('open_match').replace(t('open_match'), '') || 'online' : 'offline'}
                  <span style={{ marginLeft: 'auto', fontWeight: 400, textTransform: 'none', letterSpacing: 0 }}>
                    <RelativeTime ts={s.lastSeen ? new Date(s.lastSeen).toISOString() : null} />
                  </span>
                </div>
                <div className="esp-match">
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
