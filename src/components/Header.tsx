import Link from 'next/link';
import { getTranslations } from 'next-intl/server';
import { getSession } from '@/lib/session';
import { LangSwitch } from './LangSwitch';

export async function Header() {
  const t = await getTranslations('nav');
  const session = await getSession();
  const isAuth = !!session.userId;
  const isAdmin = session.role === 'ADMIN';

  return (
    <header className="site">
      <div className="container row">
        <Link href="/" className="brand">hmyLaser32</Link>
        <nav>
          <Link href="/wiki">{t('wiki')}</Link>
          <Link href="/leaderboard">{t('leaderboard')}</Link>
          {isAuth && <Link href="/profile">{t('profile')}</Link>}
          {isAdmin && <Link href="/admin">{t('admin')}</Link>}
          {!isAuth && <Link href="/auth/login">{t('login')}</Link>}
          {!isAuth && <Link href="/auth/register">{t('register')}</Link>}
          {isAuth && (
            <form action="/api/auth/logout" method="post" style={{ margin: 0 }}>
              <button type="submit" className="btn btn-sm">{t('logout')}</button>
            </form>
          )}
          <LangSwitch />
        </nav>
      </div>
    </header>
  );
}
