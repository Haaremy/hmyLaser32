'use client';

import { useTranslations } from 'next-intl';
import { useRouter } from 'next/navigation';
import { FormEvent, useState } from 'react';

type Tab = 'login' | 'register';

export function AccountForms() {
  const t = useTranslations('account');
  const [tab, setTab] = useState<Tab>('login');
  const router = useRouter();
  const [err, setErr] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  async function submit(e: FormEvent<HTMLFormElement>, kind: Tab) {
    e.preventDefault();
    setErr(null);
    setLoading(true);
    const fd = new FormData(e.currentTarget);
    const r = await fetch(`/api/auth/${kind}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        username: fd.get('username'),
        password: fd.get('password')
      })
    });
    setLoading(false);
    if (!r.ok) {
      const j = await r.json().catch(() => ({ error: 'server_error' }));
      setErr(j.error || 'server_error');
      return;
    }
    router.refresh();
  }

  return (
    <div className="hmy-card" style={{ maxWidth: 480 }}>
      <div className="hmy-tabs" role="tablist">
        <button
          role="tab"
          aria-selected={tab === 'login'}
          className={`hmy-tab ${tab === 'login' ? 'hmy-tab--active' : ''}`}
          onClick={() => { setTab('login'); setErr(null); }}
        >
          {t('tab_login')}
        </button>
        <button
          role="tab"
          aria-selected={tab === 'register'}
          className={`hmy-tab ${tab === 'register' ? 'hmy-tab--active' : ''}`}
          onClick={() => { setTab('register'); setErr(null); }}
        >
          {t('tab_register')}
        </button>
      </div>
      <div className="hmy-card__body">
        {err && <div className="hmy-alert hmy-alert--error">{t(`errors.${err}` as any)}</div>}
        {tab === 'login' ? (
          <form onSubmit={(e) => submit(e, 'login')}>
            <div className="hmy-field">
              <label className="hmy-field__label" htmlFor="login-username">{t('username')}</label>
              <input id="login-username" name="username" className="hmy-input" autoComplete="username" required minLength={3} maxLength={32} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label" htmlFor="login-password">{t('password')}</label>
              <input id="login-password" name="password" type="password" className="hmy-input" autoComplete="current-password" required minLength={8} maxLength={128} />
            </div>
            <button className="hmy-btn hmy-btn--primary" type="submit" disabled={loading}>{t('submit_login')}</button>
          </form>
        ) : (
          <form onSubmit={(e) => submit(e, 'register')}>
            <div className="hmy-field">
              <label className="hmy-field__label" htmlFor="reg-username">{t('username')}</label>
              <input id="reg-username" name="username" className="hmy-input" autoComplete="username" required minLength={3} maxLength={32} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label" htmlFor="reg-password">{t('password')}</label>
              <input id="reg-password" name="password" type="password" className="hmy-input" autoComplete="new-password" required minLength={8} maxLength={128} />
            </div>
            <button className="hmy-btn hmy-btn--primary" type="submit" disabled={loading}>{t('submit_register')}</button>
          </form>
        )}
      </div>
    </div>
  );
}
