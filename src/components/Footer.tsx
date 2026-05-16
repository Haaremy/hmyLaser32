import { getTranslations } from 'next-intl/server';

export async function Footer() {
  const t = await getTranslations('footer');
  return (
    <footer className="hmy-footer">
      <div className="hmy-container hmy-footer__row">
        <span>© {new Date().getFullYear()} Haaremy · {t('tagline')}</span>
        <span>
          <a href="https://legal.haaremy.de/impressum">{t('imprint')}</a>{' · '}
          <a href="https://legal.haaremy.de/datenschutz">{t('privacy')}</a>{' · '}
          <a href={process.env.NEXT_PUBLIC_GITHUB_URL || 'https://github.com/Haaremy/hmyLaser32'}>GitHub</a>
        </span>
      </div>
    </footer>
  );
}
