'use client';

import { useTranslations } from 'next-intl';
import { useRouter } from 'next/navigation';
import { FormEvent, useState } from 'react';

export function DeviceCreator() {
  const t = useTranslations('admin');
  const router = useRouter();
  const [name, setName] = useState('');
  const [issued, setIssued] = useState<{ name: string; accessCode: string } | null>(null);
  const [err, setErr] = useState<string | null>(null);

  async function onSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    setErr(null);
    const r = await fetch('/api/admin/devices', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name })
    });
    if (!r.ok) {
      setErr((await r.json().catch(() => null))?.error || 'server_error');
      return;
    }
    const d = await r.json();
    setIssued(d.server);
    setName('');
    router.refresh();
  }

  return (
    <div className="card">
      <h3 style={{ marginTop: 0 }}>{t('create_device')}</h3>
      {err && <div className="alert alert-error">{err}</div>}
      {issued && (
        <div className="alert alert-success">
          <p style={{ margin: 0 }}><strong>{issued.name}</strong></p>
          <p style={{ margin: '0.5rem 0' }}>{t('code_generated')}</p>
          <code className="code" style={{ fontSize: '1rem', wordBreak: 'break-all' }}>{issued.accessCode}</code>
        </div>
      )}
      <form onSubmit={onSubmit} style={{ display: 'flex', gap: '0.5rem' }}>
        <input
          required
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder={t('device_name')}
          maxLength={64}
        />
        <button className="btn btn-primary" type="submit">{t('create_device')}</button>
      </form>
    </div>
  );
}
