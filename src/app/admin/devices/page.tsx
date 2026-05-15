import { redirect } from 'next/navigation';
import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';
import { DeviceCreator } from './DeviceCreator';

export const dynamic = 'force-dynamic';

export default async function AdminDevicesPage() {
  const session = await getSession();
  if (!session.userId) redirect('/auth/login');
  const t = await getTranslations('admin');
  if (session.role !== 'ADMIN') return <div className="alert alert-error">{t('no_access')}</div>;

  const devices = await db.espServer.findMany({
    orderBy: { createdAt: 'desc' },
    select: { id: true, name: true, lastSeen: true, accessCode: true, createdAt: true }
  });

  return (
    <>
      <h1>{t('devices')}</h1>
      <DeviceCreator />
      <table style={{ marginTop: '2rem' }}>
        <thead>
          <tr>
            <th>{t('device_name')}</th>
            <th>{t('access_code')}</th>
            <th>{t('last_seen')}</th>
            <th>Created</th>
          </tr>
        </thead>
        <tbody>
          {devices.map((d) => (
            <tr key={d.id}>
              <td>{d.name}</td>
              <td><code className="code">{d.accessCode.slice(0, 8)}…{d.accessCode.slice(-4)}</code></td>
              <td>{d.lastSeen ? new Date(d.lastSeen).toLocaleString() : '—'}</td>
              <td>{new Date(d.createdAt).toLocaleDateString()}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </>
  );
}
