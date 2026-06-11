'use client';

import { useState } from 'react';

type Props = {
  value: string;
  copyLabel: string;
  copiedLabel: string;
};

export function NfcTokenBox({ value, copyLabel, copiedLabel }: Props) {
  const [copied, setCopied] = useState(false);

  async function copy() {
    await navigator.clipboard.writeText(value);
    setCopied(true);
    window.setTimeout(() => setCopied(false), 1800);
  }

  return (
    <div style={{ display: 'grid', gap: '0.5rem' }}>
      <code className="hmy-code" style={{ wordBreak: 'break-all', fontSize: '1rem', display: 'block', padding: '0.75rem' }}>
        {value}
      </code>
      <button className="hmy-btn hmy-btn--sm" type="button" onClick={copy}>
        {copied ? copiedLabel : copyLabel}
      </button>
    </div>
  );
}
