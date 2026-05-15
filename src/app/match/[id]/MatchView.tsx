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

  const fetchLive = useCallback(async () => {
    try {
      const r = await fetch(`/api/match/${matchId}/live`, { cache: 'no-store' });
      if (!r.ok) throw new Error(`http ${r.status}`);
      const data: LiveState = await r.json();
      // Highlight neue Feed-Einträge
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
    } catch (e: any) {
      setErr(e?.message || 'load_error');
    }
  }, [matchId]);

  useEffect(() => {
    fetchLive();
    const i = setInterval(fetchLive, 2500);
    return () => clearInterval(i);
  }, [fetchLive]);

  if (err && !state) return <div className="alert alert-error">{err}</div>;
  if (!state) return <p>{t('title')}…</p>;

  const isLive = state.match.status === 'active';
  const remaining = state.match.remainingSeconds;
  const remainingCls = remaining !== null && remaining < 60 ? 'danger' : remaining !== null && remaining < 180 ? 'warning' : '';

  return (
    <>
      <div className="match-header">
        <div>
          <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
            {state.server.name}
            {isLive && <span className="live-badge">{t('live_indicator')}</span>}
          </h1>
          <div className="match-meta">
            <span><strong>{t('mode')}:</strong> {state.match.mode || '—'}</span>
            <span><strong>{t('status')}:</strong> {state.match.status}</span>
            <span><strong>{t('started')}:</strong> {new Date(state.match.startedAt).toLocaleTimeString()}</span>
          </div>
        </div>
        {remaining !== null && (
          <div style={{ textAlign: 'right' }}>
            <div style={{ fontSize: '0.75rem', textTransform: 'uppercase', letterSpacing: '0.08em', color: 'var(--color-text-muted)', marginBottom: 4 }}>
              {t('remaining')}
            </div>
            <div className={`timer ${remainingCls}`}>{formatTime(remaining)}</div>
          </div>
        )}
      </div>

      <div className="tabs">
        <button className={tab === 'live' ? 'active' : ''} onClick={() => setTab('live')}>
          {t('tab_live')}
        </button>
        <button className={tab === 'settings' ? 'active' : ''} onClick={() => setTab('settings')}>
          {t('tab_settings')}
        </button>
      </div>

      {tab === 'live' ? (
        <div className="live-grid">
          <div>
            <h2 style={{ marginTop: 0 }}>{t('leaderboard')}</h2>
            <div className="lb-row head">
              <div>#</div>
              <div>{t('players')}</div>
              <div style={{ textAlign: 'right' }}>K/D</div>
              <div style={{ textAlign: 'right' }}>%</div>
              <div style={{ textAlign: 'right' }}>{t('shot')}</div>
              <div style={{ textAlign: 'right' }}>{t('score')}</div>
            </div>
            {state.leaderboard.length === 0 ? (
              <p style={{ color: 'var(--color-text-muted)' }}>—</p>
            ) : (
              state.leaderboard.map((p, i) => (
                <div className="lb-row" key={p.id}>
                  <div className={`lb-rank ${i === 0 ? 'gold' : i === 1 ? 'silver' : i === 2 ? 'bronze' : ''}`}>
                    {i + 1}
                  </div>
                  <div className="lb-name" style={{ borderLeftColor: p.teamColor, color: p.teamColor }}>
                    {p.name}
                    {p.teamName && (
                      <span style={{ marginLeft: 8, fontSize: '0.75em', opacity: 0.7, color: 'var(--color-text-secondary)' }}>
                        {p.teamName}
                      </span>
                    )}
                  </div>
                  <div className="lb-num">{p.kd}</div>
                  <div className="lb-num">{p.accuracy === null ? '—' : `${p.accuracy}%`}</div>
                  <div className="lb-num">{p.hits}</div>
                  <div className="lb-num lb-points">{p.points}</div>
                </div>
              ))
            )}
          </div>

          <div>
            <h2 style={{ marginTop: 0 }}>{t('live_feed')}</h2>
            <div className="feed">
              {state.feed.length === 0 ? (
                <p style={{ color: 'var(--color-text-muted)' }}>{t('no_feed')}</p>
              ) : (
                state.feed.map((f) => (
                  <div className={`feed-row ${newFeedIds.has(f.id) ? 'new' : ''}`} key={f.id}>
                    <span className="feed-ts">{new Date(f.ts).toLocaleTimeString()}</span>
                    <span className="feed-text">
                      <strong>{nameForToken(f.shooter, state.leaderboard)}</strong>
                      {' → '}
                      <strong>{nameForToken(f.target, state.leaderboard)}</strong>
                    </span>
                    <span className="feed-pts">+{f.points}</span>
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
  const [settings, setSettings] = useState<Record<string, unknown>>({});
  const [duration, setDuration] = useState<number | ''>('');
  const [mode, setMode] = useState('');
  const [extra, setExtra] = useState('{}');
  const [err, setErr] = useState<string | null>(null);
  const [msg, setMsg] = useState<string | null>(null);

  useEffect(() => {
    fetch(`/api/match/${matchId}/settings`).then(async (r) => {
      if (r.ok) {
        const d = await r.json();
        setSettings(d.settings || {});
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
      <div className="card" style={{ maxWidth: 480, margin: '0 auto' }}>
        <p>{t('settings_locked')}</p>
        <form
          onSubmit={(e) => {
            e.preventDefault();
            setUnlocked(true);
          }}
        >
          <label>
            <span>{t('settings_pin_label')}</span>
            <input
              type="password"
              autoComplete="off"
              value={pin}
              onChange={(e) => setPin(e.target.value)}
              required
              minLength={4}
              style={{ fontFamily: 'var(--font-family-mono)', letterSpacing: '0.2em' }}
            />
          </label>
          {err && <div className="alert alert-error">{err}</div>}
          <button className="btn btn-primary" type="submit">
            {t('settings_unlock')}
          </button>
        </form>
      </div>
    );
  }

  return (
    <form onSubmit={save} className="card" style={{ maxWidth: 640, margin: '0 auto' }}>
      <label>
        <span>{t('settings_mode')}</span>
        <input value={mode} onChange={(e) => setMode(e.target.value)} placeholder="team-deathmatch" />
      </label>
      <label>
        <span>{t('settings_duration')}</span>
        <input type="number" min={30} max={86400} value={duration} onChange={(e) => setDuration(e.target.value === '' ? '' : Number(e.target.value))} />
      </label>
      <label>
        <span>{t('settings_extra')}</span>
        <textarea rows={6} value={extra} onChange={(e) => setExtra(e.target.value)} style={{ fontFamily: 'var(--font-family-mono)', fontSize: '0.85rem' }} />
      </label>
      <label>
        <span>{t('settings_pin_label')}</span>
        <input
          type="password"
          autoComplete="off"
          value={pin}
          onChange={(e) => setPin(e.target.value)}
          required
          minLength={4}
          style={{ fontFamily: 'var(--font-family-mono)', letterSpacing: '0.2em' }}
        />
      </label>
      {err && <div className="alert alert-error">{err}</div>}
      {msg && <div className="alert alert-success">{msg}</div>}
      <button className="btn btn-primary" type="submit">
        {t('settings_save')}
      </button>
    </form>
  );
}
