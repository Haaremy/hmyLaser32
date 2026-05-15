'use client';

import { useLocale, useTranslations } from 'next-intl';
import { useRouter } from 'next/navigation';
import { useTransition } from 'react';

export function LangSwitch() {
  const locale = useLocale();
  const t = useTranslations('common');
  const router = useRouter();
  const [, startTransition] = useTransition();

  function setLocale(next: string) {
    document.cookie = `NEXT_LOCALE=${next}; path=/; max-age=31536000; SameSite=Lax`;
    startTransition(() => router.refresh());
  }

  return (
    <span className="lang-switch" aria-label={t('language')}>
      <button type="button" className={locale === 'de' ? 'active' : ''} onClick={() => setLocale('de')}>DE</button>{' '}
      <button type="button" className={locale === 'en' ? 'active' : ''} onClick={() => setLocale('en')}>EN</button>
    </span>
  );
}
