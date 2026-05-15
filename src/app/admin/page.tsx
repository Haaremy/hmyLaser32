import Link from 'next/link';
import { redirect } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { getSession } from '@/lib/session';

export const dynamic = 'force-dynamic';

export default async function AdminHome() {
  const session = await getSession();
  if (!session.userId) redirect('/auth/login');
  const t = await getTranslations('admin');
  if (session.role !== 'ADMIN') {
    return <div className="alert alert-error">{t('no_access')}</div>;
  }
  return (
    <>
      <h1>{t('title')}</h1>
      <div className="grid grid-3">
        <Link href="/admin/users" className="feature"><h3>👥 {t('users')}</h3></Link>
        <Link href="/admin/devices" className="feature"><h3>📡 {t('devices')}</h3></Link>
        <Link href="/admin/matches" className="feature"><h3>🎯 {t('matches')}</h3></Link>
      </div>
    </>
  );
}
