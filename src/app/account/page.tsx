import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';
import { AccountForms } from './AccountForms';

export const dynamic = 'force-dynamic';

export default async function AccountPage() {
  const session = await getSession();
  const t = await getTranslations('account');

  if (!session.userId) {
    return (
      <>
        <h1>{t('title')}</h1>
        <p>{t('subtitle_logged_out')}</p>
        <AccountForms />
      </>
    );
  }

  const user = await db.user.findUnique({
    where: { id: session.userId },
    select: { id: true, username: true, role: true, nfcToken: true, createdAt: true }
  });
  if (!user) {
    return (
      <>
        <h1>{t('title')}</h1>
        <AccountForms />
      </>
    );
  }

  const players = await db.matchPlayer.findMany({
    where: { userId: user.id },
    include: { match: { select: { id: true, startedAt: true, endedAt: true, mode: true, status: true } } },
    orderBy: { match: { startedAt: 'desc' } },
    take: 25
  });

  const totals = players.reduce(
    (acc, p) => ({
      hits: acc.hits + p.hits,
      deaths: acc.deaths + p.deaths,
      points: acc.points + p.points,
      shots: acc.shots + p.shotsFired
    }),
    { hits: 0, deaths: 0, points: 0, shots: 0 }
  );
  const kd = totals.deaths === 0 ? totals.hits.toFixed(2) : (totals.hits / totals.deaths).toFixed(2);
  const accuracy = totals.shots === 0 ? '—' : ((totals.hits / totals.shots) * 100).toFixed(1) + ' %';

  return (
    <>
      <h1>{t('title')}</h1>

      <section className="hmy-card">
        <div className="hmy-card__header">{t('profile_heading')}</div>
        <div className="hmy-card__body">
          <div className="hmy-grid hmy-grid--2">
            <div>
              <div style={{ fontSize: 'var(--hmy-font-size-xs)', textTransform: 'uppercase', color: 'var(--hmy-color-text-muted)', letterSpacing: '0.04em' }}>
                {t('username')}
              </div>
              <div style={{ fontWeight: 600, fontSize: 'var(--hmy-font-size-lg)' }}>{user.username}</div>
            </div>
            <div>
              <div style={{ fontSize: 'var(--hmy-font-size-xs)', textTransform: 'uppercase', color: 'var(--hmy-color-text-muted)', letterSpacing: '0.04em' }}>
                {t('role')}
              </div>
              <span className={`hmy-lt-pill ${user.role === 'ADMIN' ? 'hmy-lt-pill--warning' : 'hmy-lt-pill--muted'}`}>{user.role}</span>
            </div>
          </div>
          <h3>{t('nfc_token')}</h3>
          <code className="hmy-code" style={{ wordBreak: 'break-all', fontSize: '1rem', display: 'block', padding: '0.75rem' }}>{user.nfcToken}</code>
          <p style={{ marginTop: '0.5rem', fontSize: 'var(--hmy-font-size-sm)' }}>{t('nfc_hint')}</p>
        </div>
      </section>

      <section className="hmy-card">
        <div className="hmy-card__header">{t('stats_heading')}</div>
        <div className="hmy-card__body">
          <div className="hmy-grid hmy-grid--3">
            <StatBox label={t('matches_played')} value={players.length} />
            <StatBox label={t('total_hits')} value={totals.hits} />
            <StatBox label={t('total_deaths')} value={totals.deaths} />
            <StatBox label={t('total_points')} value={totals.points} />
            <StatBox label={t('kd_ratio')} value={kd} />
            <StatBox label={t('accuracy')} value={accuracy} />
          </div>
        </div>
      </section>

      <h2>{t('history_heading')}</h2>
      {players.length === 0 ? (
        <p>{t('no_matches')}</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>Match</th>
              <th>Mode</th>
              <th className="num">Hits</th>
              <th className="num">Deaths</th>
              <th className="num">Points</th>
            </tr>
          </thead>
          <tbody>
            {players.map((p) => (
              <tr key={p.id}>
                <td>
                  <a href={`/match/${p.match.id}`}>
                    {new Date(p.match.startedAt).toLocaleString()}
                  </a>
                </td>
                <td>{p.match.mode || '—'}</td>
                <td className="num">{p.hits}</td>
                <td className="num">{p.deaths}</td>
                <td className="num">{p.points}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </>
  );
}

function StatBox({ label, value }: { label: string; value: string | number }) {
  return (
    <div className="hmy-lt-recipe__meta-item">
      <div className="hmy-lt-recipe__meta-label">{label}</div>
      <div className="hmy-lt-recipe__meta-value" style={{ fontSize: 'var(--hmy-font-size-2xl)', fontFamily: 'var(--hmy-font-family-mono)' }}>
        {value}
      </div>
    </div>
  );
}
