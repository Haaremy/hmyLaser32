'use client';

import { useTranslations } from 'next-intl';
import { useCallback, useEffect, useRef, useState } from 'react';

type Player = {
  id: string;
  nfcToken: string;
  name: string;
  teamName: string | null;
  teamColor: string;
  hits: number;
  deaths: number;
  shotsFired: number;
  points: number;
  kd: number;
  accuracy: number | null;
};

type FeedItem = { id: string; ts: string; shooter: string | null; target: string | null; points: number };

type LiveState = {
  match: {
    id: string;
    mode: string | null;
    status: string;
    startedAt: string;
    endedAt: string | null;
    durationSeconds: number | null;
    remainingSeconds: number | null;
  };
  server: { name: string; online: boolean; lastSeen: string | null };
  teams: Array<{ id: string; name: string; color: string; totalPoints: number }>;
  leaderboard: Player[];
  feed: FeedItem[];
};

function formatTime(seconds: number | null): string {
  if (seconds === null) return '—';
  const m = Math.floor(seconds / 60);
  const s = seconds % 60;
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

function nameForToken(token: string | null, leaderboard: Player[]): string {
  if (!token) return '?';
  const p = leaderboard.find((x) => x.nfcToken === token);
  return p ? p.name : `nfc:${token.slice(0, 6)}`;
}

export function MatchView({ matchId }: { matchId: string }) {
  const t = useTranslations('match');
  const [tab, setTab] = useState<'live' | 'settings'>('live');
  const [state, setState] = useState<LiveState | null>(null);
  const [err, setErr] = useState<string | null>(null);
  const [newFeedIds, setNewFeedIds] = useState<Set<string>>(new Set());
  const seenIdsRef = useRef<Set<string>>(new Set());
  const wsRef = useRef<WebSocket | null>(null);

  const applySnapshot = useCallback((data: LiveState) => {
    const fresh = new Set<string>();
    for (const f of data.feed) {
      if (!seenIdsRef.current.has(f.id)) fresh.add(f.id);
      seenIdsRef.current.add(f.id);
    }
    if (fresh.size > 0) {
      setNewFeedIds(fresh);
      setTimeout(() => setNewFeedIds(new Set()), 1200);
    }
    setState(data);
    setErr(null);
  }, []);

  useEffect(() => {
    let stopped = false;
    let reconnectTimer: ReturnType<typeof setTimeout> | null = null;

    function connect() {
      if (stopped) return;
      const proto = window.location.protocol === 'https:' ? 'wss' : 'ws';
      const url = `${proto}://${window.location.host}/api/ws/match/${matchId}`;
      const ws = new WebSocket(url);
      wsRef.current = ws;
      ws.onmessage = (ev) => {
        try {
          const msg = JSON.parse(ev.data);
          if (msg.type === 'snapshot') applySnapshot(msg as any);
        } catch {}
      };
      ws.onerror = () => setErr('socket_error');
      ws.onclose = () => {
        wsRef.current = null;
        if (!stopped) reconnectTimer = setTimeout(connect, 2000);
      };
    }
    connect();
    return () => {
      stopped = true;
      if (reconnectTimer) clearTimeout(reconnectTimer);
      if (wsRef.current) wsRef.current.close();
    };
  }, [matchId, applySnapshot]);

  if (err && !state) return <div className="hmy-alert hmy-alert--error">{err}</div>;
  if (!state) return <p>{t('title')}…</p>;

  const isLive = state.match.status === 'active';
  const remaining = state.match.remainingSeconds;
  let timerCls = '';
  if (remaining !== null && remaining < 60) timerCls = 'is-danger';
  else if (remaining !== null && remaining < 180) timerCls = 'is-warning';

  return (
    <>
      <div className="hmy-lt-match-header">
        <div>
          <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
            {state.server.name}
            {isLive && <span className="hmy-lt-live-badge">{t('live_indicator')}</span>}
          </h1>
          <div className="hmy-lt-match-meta">
            <span><strong>{t('mode')}:</strong> {state.match.mode || '—'}</span>
            <span><strong>{t('status')}:</strong> {state.match.status}</span>
            <span><strong>{t('started')}:</strong> {new Date(state.match.startedAt).toLocaleTimeString()}</span>
          </div>
        </div>
        {remaining !== null && (
          <div style={{ textAlign: 'right' }}>
            <div style={{ fontSize: 'var(--hmy-font-size-xs)', textTransform: 'uppercase', letterSpacing: '0.08em', color: 'var(--hmy-color-text-muted)', marginBottom: 4 }}>
              {t('remaining')}
            </div>
            <div className={`hmy-lt-timer ${timerCls}`}>{formatTime(remaining)}</div>
          </div>
        )}
      </div>

      <div className="hmy-tabs" role="tablist">
        <button
          role="tab"
          aria-selected={tab === 'live'}
          className={`hmy-tab ${tab === 'live' ? 'hmy-tab--active' : ''}`}
          onClick={() => setTab('live')}
        >
          {t('tab_live')}
        </button>
        <button
          role="tab"
          aria-selected={tab === 'settings'}
          className={`hmy-tab ${tab === 'settings' ? 'hmy-tab--active' : ''}`}
          onClick={() => setTab('settings')}
        >
          {t('tab_settings')}
        </button>
      </div>

      {tab === 'live' ? (
        <div className="hmy-lt-live-grid">
          <div>
            <h2 style={{ marginTop: 0 }}>{t('leaderboard')}</h2>
            <div className="hmy-lt-lb-row is-head">
              <div>#</div>
              <div>{t('players')}</div>
              <div style={{ textAlign: 'right' }}>Shots</div>
              <div style={{ textAlign: 'right' }}>RX Hits</div>
              <div style={{ textAlign: 'right' }}>{t('score')}</div>
            </div>
            {state.leaderboard.length === 0 ? (
              <p style={{ color: 'var(--hmy-color-text-muted)' }}>—</p>
            ) : (
              state.leaderboard.map((p, i) => (
                <div className="hmy-lt-lb-row" key={p.id}>
                  <div className={`hmy-lt-rank ${i === 0 ? 'is-gold' : i === 1 ? 'is-silver' : i === 2 ? 'is-bronze' : ''}`}>
                    {i + 1}
                  </div>
                  <div className="hmy-lt-lb-name" style={{ borderLeftColor: p.teamColor, color: p.teamColor }}>
                    {p.name}
                    {p.teamName && (
                      <span style={{ marginLeft: 8, fontSize: '0.75em', opacity: 0.7, color: 'var(--hmy-color-text-secondary)' }}>
                        {p.teamName}
                      </span>
                    )}
                  </div>
                  <div className="hmy-lt-num">{p.shotsFired}</div>
                  <div className="hmy-lt-num">{p.deaths}</div>
                  <div className="hmy-lt-num hmy-lt-points">{p.points}</div>
                </div>
              ))
            )}
          </div>

          <div>
            <h2 style={{ marginTop: 0 }}>{t('live_feed')}</h2>
            <div className="hmy-lt-feed">
              {state.feed.length === 0 ? (
                <p style={{ color: 'var(--hmy-color-text-muted)' }}>{t('no_feed')}</p>
              ) : (
                state.feed.map((f) => (
                  <div className={`hmy-lt-feed__row ${newFeedIds.has(f.id) ? 'is-new' : ''}`} key={f.id}>
                    <span className="hmy-lt-feed__ts">{new Date(f.ts).toLocaleTimeString()}</span>
                    <span className="hmy-lt-feed__text">
                      <strong>{nameForToken(f.shooter, state.leaderboard)}</strong>
                      {' → '}
                      <strong>{nameForToken(f.target, state.leaderboard)}</strong>
                    </span>
                    <span className="hmy-lt-feed__pts">+{f.points}</span>
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      ) : (
        <SettingsTab matchId={matchId} />
      )}
    </>
  );
}

function SettingsTab({ matchId }: { matchId: string }) {
  const t = useTranslations('match');
  const [pin, setPin] = useState('');
  const [unlocked, setUnlocked] = useState(false);
  const [duration, setDuration] = useState<number | ''>('');
  const [mode, setMode] = useState('');
  const [extra, setExtra] = useState('{}');
  const [err, setErr] = useState<string | null>(null);
  const [msg, setMsg] = useState<string | null>(null);

  useEffect(() => {
    fetch(`/api/match/${matchId}/settings`).then(async (r) => {
      if (r.ok) {
        const d = await r.json();
        setMode(d.mode || '');
        setDuration(d.durationSeconds ?? '');
        setExtra(JSON.stringify(d.settings || {}, null, 2));
      }
    });
  }, [matchId]);

  async function save(e: React.FormEvent) {
    e.preventDefault();
    setErr(null);
    setMsg(null);
    let parsedExtra: Record<string, unknown> = {};
    try {
      parsedExtra = extra ? JSON.parse(extra) : {};
    } catch {
      setErr('JSON parse error');
      return;
    }
    const r = await fetch(`/api/match/${matchId}/settings`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ pin, settings: { ...parsedExtra, mode, durationSeconds: duration } })
    });
    if (!r.ok) {
      setErr(r.status === 403 ? t('settings_invalid_pin') : 'server_error');
      setUnlocked(false);
      return;
    }
    setMsg(t('settings_saved'));
    setUnlocked(true);
  }

  if (!unlocked) {
    return (
      <div className="hmy-card" style={{ maxWidth: 480, margin: '0 auto' }}>
        <div className="hmy-card__body">
          <p>{t('settings_locked')}</p>
          <form onSubmit={(e) => { e.preventDefault(); setUnlocked(true); }}>
            <div className="hmy-field">
              <label className="hmy-field__label">{t('settings_pin_label')}</label>
              <input
                type="password"
                autoComplete="off"
                className="hmy-input"
                value={pin}
                onChange={(e) => setPin(e.target.value)}
                required
                minLength={4}
                style={{ fontFamily: 'var(--hmy-font-family-mono)', letterSpacing: '0.2em' }}
              />
            </div>
            {err && <div className="hmy-alert hmy-alert--error">{err}</div>}
            <button className="hmy-btn hmy-btn--primary" type="submit">{t('settings_unlock')}</button>
          </form>
        </div>
      </div>
    );
  }

  return (
    <form onSubmit={save} className="hmy-card" style={{ maxWidth: 640, margin: '0 auto' }}>
      <div className="hmy-card__body">
        <div className="hmy-field">
          <label className="hmy-field__label">{t('settings_mode')}</label>
          <input className="hmy-input" value={mode} onChange={(e) => setMode(e.target.value)} placeholder="team-deathmatch" />
        </div>
        <div className="hmy-field">
          <label className="hmy-field__label">{t('settings_duration')}</label>
          <input
            className="hmy-input"
            type="number"
            min={30}
            max={86400}
            value={duration}
            onChange={(e) => setDuration(e.target.value === '' ? '' : Number(e.target.value))}
          />
        </div>
        <div className="hmy-field">
          <label className="hmy-field__label">{t('settings_extra')}</label>
          <textarea
            className="hmy-textarea"
            rows={6}
            value={extra}
            onChange={(e) => setExtra(e.target.value)}
            style={{ fontFamily: 'var(--hmy-font-family-mono)', fontSize: '0.85rem' }}
          />
        </div>
        <div className="hmy-field">
          <label className="hmy-field__label">{t('settings_pin_label')}</label>
          <input
            className="hmy-input"
            type="password"
            autoComplete="off"
            value={pin}
            onChange={(e) => setPin(e.target.value)}
            required
            minLength={4}
            style={{ fontFamily: 'var(--hmy-font-family-mono)', letterSpacing: '0.2em' }}
          />
        </div>
        {err && <div className="hmy-alert hmy-alert--error">{err}</div>}
        {msg && <div className="hmy-alert hmy-alert--success">{msg}</div>}
      </div>
      <div className="hmy-card__footer">
        <button className="hmy-btn hmy-btn--primary" type="submit">{t('settings_save')}</button>
      </div>
    </form>
  );
}
