import Link from 'next/link';
import { getTranslations } from 'next-intl/server';
import { getSession } from '@/lib/session';
import { LangSwitch } from './LangSwitch';
import { NavLink } from './NavLink';

export async function Header() {
  const t = await getTranslations('nav');
  const tc = await getTranslations('common');
  const session = await getSession();
  const isAuth = !!session.userId;

  return (
    <header className="hmy-header">
      <div className="hmy-container hmy-header__row">
        <Link href="/" className="hmy-header__brand">hmyLaser32</Link>
        <nav className="hmy-header__nav" aria-label="Primary">
          <NavLink href="/">{t('home')}</NavLink>
          <NavLink href="/research">{t('research')}</NavLink>
          <NavLink href="/diy">{t('diy')}</NavLink>
          <NavLink href="/champions">{t('champions')}</NavLink>
          <NavLink href="/account">{t('account')}</NavLink>
          {isAuth && (
            <form action="/api/auth/logout" method="post" style={{ margin: 0 }}>
              <button type="submit" className="hmy-btn hmy-btn--ghost hmy-btn--sm">{tc('logout')}</button>
            </form>
          )}
          <LangSwitch />
        </nav>
      </div>
    </header>
  );
}
