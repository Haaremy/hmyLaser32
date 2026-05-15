import { redirect } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';

export const dynamic = 'force-dynamic';

export default async function AdminUsersPage() {
  const session = await getSession();
  if (!session.userId) redirect('/auth/login');
  const t = await getTranslations('admin');
  if (session.role !== 'ADMIN') return <div className="alert alert-error">{t('no_access')}</div>;

  const users = await db.user.findMany({
    select: {
      id: true,
      username: true,
      role: true,
      nfcToken: true,
      createdAt: true,
      _count: { select: { matches: true } }
    },
    orderBy: { createdAt: 'desc' }
  });

  return (
    <>
      <h1>{t('users')}</h1>
      <table>
        <thead>
          <tr><th>Username</th><th>Role</th><th>NFC</th><th>Matches</th><th>Created</th></tr>
        </thead>
        <tbody>
          {users.map((u) => (
            <tr key={u.id}>
              <td>{u.username}</td>
              <td>{u.role}</td>
              <td><code className="code">{u.nfcToken.slice(0, 8)}…</code></td>
              <td>{u._count.matches}</td>
              <td>{new Date(u.createdAt).toLocaleDateString()}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </>
  );
}
