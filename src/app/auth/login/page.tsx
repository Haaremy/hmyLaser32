'use client';

import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { useTranslations } from 'next-intl';
import { FormEvent, useState } from 'react';

export default function LoginPage() {
  const t = useTranslations('auth');
  const router = useRouter();
  const [err, setErr] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  async function onSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    setLoading(true);
    setErr(null);
    const fd = new FormData(e.currentTarget);
    const r = await fetch('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        username: fd.get('username'),
        password: fd.get('password')
      })
    });
    if (!r.ok) {
      const j = await r.json().catch(() => ({ error: 'server_error' }));
      setErr(j.error || 'server_error');
      setLoading(false);
      return;
    }
    router.push('/profile');
    router.refresh();
  }

  return (
    <div className="form">
      <h1>{t('login_title')}</h1>
      {err && <div className="alert alert-error">{t(`errors.${err}` as any)}</div>}
      <form onSubmit={onSubmit}>
        <label>
          <span>{t('username')}</span>
          <input name="username" required autoComplete="username" />
        </label>
        <label>
          <span>{t('password')}</span>
          <input name="password" type="password" required autoComplete="current-password" />
        </label>
        <button className="btn btn-primary" type="submit" disabled={loading}>
          {t('submit_login')}
        </button>
      </form>
      <p style={{ marginTop: '1rem' }}>
        {t('no_account')} <Link href="/auth/register">{t('register_title')}</Link>
      </p>
    </div>
  );
}
