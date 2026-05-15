'use client';

import { useEffect, useState } from 'react';
import { useLocale } from 'next-intl';

function format(ts: string | null, locale: string): string {
  if (!ts) return locale === 'de' ? 'noch nie' : 'never';
  const diff = (Date.now() - new Date(ts).getTime()) / 1000;
  if (diff < 5) return locale === 'de' ? 'jetzt' : 'now';
  if (diff < 60) return `${Math.floor(diff)}s`;
  if (diff < 3600) return `${Math.floor(diff / 60)}min`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h`;
  return `${Math.floor(diff / 86400)}d`;
}

export function RelativeTime({ ts }: { ts: string | null }) {
  const locale = useLocale();
  const [label, setLabel] = useState(() => format(ts, locale));
  useEffect(() => {
    setLabel(format(ts, locale));
    const i = setInterval(() => setLabel(format(ts, locale)), 30000);
    return () => clearInterval(i);
  }, [ts, locale]);
  return <span suppressHydrationWarning>{label}</span>;
}
