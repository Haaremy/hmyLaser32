'use client';

import Link from 'next/link';
import { useState } from 'react';
import { useTranslations } from 'next-intl';
import { RelativeTime } from '@/components/RelativeTime';

type Match = { id: string; startedAt: string; status: string; mode: string | null; durationSeconds: number | null };

type Props = {
  id: string;
  name: string;
  online: boolean;
  lastSeen: string | null;
  mode: string;
  lobbySeconds: number;
  matchSeconds: number;
  startRequested: boolean;
  matches: Match[];
};

export function EspDetail(p: Props) {
  const tm = useTranslations('match');
  const tc = useTranslations('common');
  const [tab, setTab] = useState<'overview' | 'settings'>('overview');

  return (
    <>
      <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', fontFamily: 'var(--hmy-font-family-mono)' }}>
        {p.name}
      </h1>
      <p>
        <span className={`hmy-lt-pill ${p.online ? 'hmy-lt-pill--success' : 'hmy-lt-pill--muted'}`}>
          {p.online ? tc('online') : tc('offline')}
        </span>{' '}
        <span style={{ color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
          · <RelativeTime ts={p.lastSeen} />
        </span>
      </p>

      <div className="hmy-tabs" role="tablist">
        <button
          role="tab"
          aria-selected={tab === 'overview'}
          className={`hmy-tab ${tab === 'overview' ? 'hmy-tab--active' : ''}`}
          onClick={() => setTab('overview')}
        >
          Übersicht
        </button>
        <button
          role="tab"
          aria-selected={tab === 'settings'}
          className={`hmy-tab ${tab === 'settings' ? 'hmy-tab--active' : ''}`}
          onClick={() => setTab('settings')}
        >
          Einstellungen
        </button>
      </div>

      {tab === 'overview' ? (
        <Overview p={p} tm={tm} />
      ) : (
        <Settings p={p} />
      )}
    </>
  );
}

function Overview({ p, tm }: { p: Props; tm: ReturnType<typeof useTranslations> }) {
  const active = p.matches.find((m) => m.status === 'active');
  return (
    <>
      {active && (
        <div className="hmy-card">
          <div className="hmy-card__header">Aktives Match</div>
          <div className="hmy-card__body">
            <Link href={`/match/${active.id}`} className="hmy-btn hmy-btn--primary">{tm('title')} →</Link>
          </div>
        </div>
      )}
      <h2>Matches ({p.matches.length})</h2>
      {p.matches.length === 0 ? (
        <p>—</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>{tm('started')}</th>
              <th>{tm('mode')}</th>
              <th>{tm('status')}</th>
              <th className="num">{tm('duration')}</th>
            </tr>
          </thead>
          <tbody>
            {p.matches.map((m) => (
              <tr key={m.id}>
                <td><Link href={`/match/${m.id}`}>{new Date(m.startedAt).toLocaleString()}</Link></td>
                <td>{m.mode || '—'}</td>
                <td>{m.status}</td>
                <td className="num">{m.durationSeconds ? `${m.durationSeconds}s` : '—'}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </>
  );
}

function Settings({ p }: { p: Props }) {
  const [mode, setMode] = useState(p.mode);
  const [lobby, setLobby] = useState(p.lobbySeconds);
  const [match, setMatch] = useState(p.matchSeconds);
  const [pin, setPin] = useState('');
  const [msg, setMsg] = useState<string | null>(null);
  const [err, setErr] = useState<string | null>(null);
  const [startPending, setStartPending] = useState(p.startRequested);

  async function saveSettings(e: React.FormEvent) {
    e.preventDefault();
    setMsg(null);
    setErr(null);
    const r = await fetch(`/api/esp/${p.id}/settings`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ pin, mode, lobbySeconds: lobby, matchSeconds: match })
    });
    if (!r.ok) {
      const j = await r.json().catch(() => null);
      setErr(j?.error === 'invalid_pin' ? 'Ungültiger PIN.' : 'Serverfehler.');
      return;
    }
    setMsg('Gespeichert. ESP übernimmt die Werte beim nächsten Pull (~30 s).');
  }

  async function startMatch() {
    setMsg(null);
    setErr(null);
    if (pin.length < 4) { setErr('Bitte PIN eingeben.'); return; }
    const r = await fetch(`/api/esp/${p.id}/start`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ pin })
    });
    if (!r.ok) {
      const j = await r.json().catch(() => null);
      setErr(j?.error === 'invalid_pin' ? 'Ungültiger PIN.' : 'Serverfehler.');
      return;
    }
    setStartPending(true);
    setMsg('Match-Start angefordert. ESP startet bei nächstem Pull (~30 s).');
  }

  return (
    <div style={{ maxWidth: 640 }}>
      <section className="hmy-card">
        <div className="hmy-card__header">Match-Defaults</div>
        <div className="hmy-card__body">
          <p style={{ marginTop: 0, color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
            Diese Werte werden vom ESP regelmäßig gepulled und für jedes neue Match verwendet.
          </p>
          <form onSubmit={saveSettings}>
            <div className="hmy-field">
              <label className="hmy-field__label">Modus</label>
              <input className="hmy-input" value={mode} onChange={(e) => setMode(e.target.value)} maxLength={23} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">Lobby-Sekunden (Verteilphase)</label>
              <input className="hmy-input" type="number" min={5} max={600} value={lobby} onChange={(e) => setLobby(Number(e.target.value))} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">Match-Sekunden</label>
              <input className="hmy-input" type="number" min={30} max={3600} value={match} onChange={(e) => setMatch(Number(e.target.value))} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">PIN des ESP-Servers</label>
              <input
                className="hmy-input"
                type="password"
                value={pin}
                onChange={(e) => setPin(e.target.value)}
                minLength={4}
                maxLength={8}
                required
                style={{ fontFamily: 'var(--hmy-font-family-mono)', letterSpacing: '0.2em' }}
              />
            </div>
            {err && <div className="hmy-alert hmy-alert--error">{err}</div>}
            {msg && <div className="hmy-alert hmy-alert--success">{msg}</div>}
            <button className="hmy-btn hmy-btn--primary" type="submit">Speichern</button>
          </form>
        </div>
      </section>

      <section className="hmy-card">
        <div className="hmy-card__header">Match starten</div>
        <div className="hmy-card__body">
          <p style={{ marginTop: 0, color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
            Setzt ein Match-Start-Flag, das der ESP-Server beim nächsten Pull (~30 s) aufnimmt.
            Anschließend startet die Lobby-Phase mit den oben gesetzten Werten.
          </p>
          {startPending && (
            <div className="hmy-alert hmy-alert--info">
              Match-Start steht aus — ESP wird ihn beim nächsten Pull übernehmen.
            </div>
          )}
          <button className="hmy-btn hmy-btn--primary" type="button" onClick={startMatch}>
            🎯 Match jetzt starten
          </button>
        </div>
      </section>
    </div>
  );
}
