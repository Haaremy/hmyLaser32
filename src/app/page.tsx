import Link from 'next/link';
import { getTranslations } from 'next-intl/server';

export default async function HomePage() {
  const t = await getTranslations('home');
  const githubUrl = process.env.NEXT_PUBLIC_GITHUB_URL || 'https://github.com/Haaremy/hmyLaser32';

  return (
    <>
      <section className="hero">
        <h1>{t('title')}</h1>
        <p className="subtitle">{t('subtitle')}</p>
        <p style={{ maxWidth: 720, margin: '0 auto 2rem' }}>{t('intro')}</p>
        <div className="cta">
          <Link className="btn btn-primary" href="/wiki">{t('cta_wiki')}</Link>
          <Link className="btn" href="/auth/register">{t('cta_register')}</Link>
          <a className="btn" href={githubUrl} rel="noopener noreferrer">GitHub</a>
        </div>
      </section>

      <section className="grid grid-2">
        <div className="feature">
          <h3>📖 Wiki</h3>
          <p>{t('features.wiki')}</p>
        </div>
        <div className="feature">
          <h3>📡 Bridge</h3>
          <p>{t('features.bridge')}</p>
        </div>
        <div className="feature">
          <h3>👤 Profile</h3>
          <p>{t('features.profile')}</p>
        </div>
        <div className="feature">
          <h3>🏆 Leaderboard</h3>
          <p>{t('features.leaderboard')}</p>
        </div>
      </section>
    </>
  );
}
