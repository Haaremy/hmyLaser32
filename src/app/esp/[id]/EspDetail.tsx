'use client';

import Link from 'next/link';
import { useEffect, useMemo, useRef, useState } from 'react';
import { useTranslations } from 'next-intl';
import { RelativeTime } from '@/components/RelativeTime';
import type { TeamDef } from '@/lib/teams';
import type { KnownPlayer } from '@/lib/players';

type Match = { id: string; startedAt: string; status: string; mode: string | null; durationSeconds: number | null };
type Mode = 'free-for-all' | 'team';

type Props = {
  id: string;
  name: string;
  online: boolean;
  lastSeen: string | null;
  mode: string;
  lobbySeconds: number;
  matchSeconds: number;
  zone1Points: number;
  zone2Points: number;
  zone3Points: number;
  teams: TeamDef[];
  knownPlayers: KnownPlayer[];
  startRequested: boolean;
  matches: Match[];
};

const DEFAULT_COLORS = ['#dc2626', '#2563eb', '#16a34a', '#d97706'];

export function EspDetail(p: Props) {
  const tm = useTranslations('match');
  const tc = useTranslations('common');
  const [tab, setTab] = useState<'overview' | 'settings'>('overview');

  // WebSocket: live snapshot + write-channel
  const [snapshot, setSnapshot] = useState<Props>(p);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    let stopped = false;
    let timer: ReturnType<typeof setTimeout> | null = null;
    function connect() {
      if (stopped) return;
      const proto = window.location.protocol === 'https:' ? 'wss' : 'ws';
      const ws = new WebSocket(`${proto}://${window.location.host}/api/ws/esp/${p.id}`);
      wsRef.current = ws;
      ws.onmessage = (ev) => {
        try {
          const msg = JSON.parse(ev.data);
          if (msg.type === 'snapshot') {
            setSnapshot((prev) => ({
              ...prev,
              online: msg.online,
              lastSeen: msg.lastSeen,
              mode: msg.mode,
              lobbySeconds: msg.lobbySeconds,
              matchSeconds: msg.matchSeconds,
              zone1Points: msg.zone1Points ?? prev.zone1Points,
              zone2Points: msg.zone2Points ?? prev.zone2Points,
              zone3Points: msg.zone3Points ?? prev.zone3Points,
              teams: msg.teams || [],
              knownPlayers: msg.knownPlayers || [],
              startRequested: msg.startRequested
            }));
          }
        } catch {}
      };
      ws.onclose = () => {
        wsRef.current = null;
        if (!stopped) timer = setTimeout(connect, 2000);
      };
    }
    connect();
    return () => {
      stopped = true;
      if (timer) clearTimeout(timer);
      if (wsRef.current) wsRef.current.close();
    };
  }, [p.id]);

  return (
    <>
      <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', fontFamily: 'var(--hmy-font-family-mono)' }}>
        {snapshot.name}
      </h1>
      <p>
        <span className={`hmy-lt-pill ${snapshot.online ? 'hmy-lt-pill--success' : 'hmy-lt-pill--muted'}`}>
          {snapshot.online ? tc('online') : tc('offline')}
        </span>{' '}
        <span style={{ color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
          · <RelativeTime ts={snapshot.lastSeen} />
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

      {tab === 'overview' ? <Overview p={snapshot} tm={tm} /> : <Settings p={snapshot} ws={wsRef} />}
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
      {p.knownPlayers.length > 0 && (
        <div className="hmy-card">
          <div className="hmy-card__header">Verbundene Clients ({p.knownPlayers.length})</div>
          <div className="hmy-card__body">
            <table>
              <thead>
                <tr><th>Name</th><th className="num">NEC-Cmd</th><th className="num">Punkte</th><th>Zuletzt gesehen</th></tr>
              </thead>
              <tbody>
                {p.knownPlayers.map((kp) => (
                  <tr key={kp.command}>
                    <td><code className="hmy-code">{kp.name}</code></td>
                    <td className="num">{kp.command}</td>
                    <td className="num">{kp.points}</td>
                    <td className="num">{kp.lastSeenSec}s</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}
      <h2>{tm('title')} ({p.matches.length})</h2>
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

function Settings({ p, ws }: { p: Props; ws: React.MutableRefObject<WebSocket | null> }) {
  const [mode, setMode] = useState<Mode>((p.mode === 'team' ? 'team' : 'free-for-all'));
  const [starttimer, setStarttimer] = useState(p.lobbySeconds);
  const [duration, setDuration] = useState(p.matchSeconds);
  const [zone1, setZone1] = useState(p.zone1Points);
  const [zone2, setZone2] = useState(p.zone2Points);
  const [zone3, setZone3] = useState(p.zone3Points);

  const [teams, setTeams] = useState<TeamDef[]>(
    p.teams.length > 0
      ? p.teams
      : [
          { name: 'Rot', color: DEFAULT_COLORS[0], members: [] },
          { name: 'Blau', color: DEFAULT_COLORS[1], members: [] }
        ]
  );

  const knownPlayers = p.knownPlayers; // live via WS

  // Re-init local form when server-side snapshot changes (only if user not actively editing)
  useEffect(() => {
    setMode((p.mode === 'team' ? 'team' : 'free-for-all'));
    setStarttimer(p.lobbySeconds);
    setDuration(p.matchSeconds);
    setZone1(p.zone1Points);
    setZone2(p.zone2Points);
    setZone3(p.zone3Points);
    if (p.teams.length > 0) setTeams(p.teams);
  }, [p.mode, p.lobbySeconds, p.matchSeconds, p.zone1Points, p.zone2Points, p.zone3Points, p.teams]);

  const allCommands = useMemo(() => {
    const set = new Set<number>();
    knownPlayers.forEach((kp) => set.add(kp.command));
    teams.forEach((t) => t.members.forEach((m) => set.add(m)));
    return Array.from(set).sort((a, b) => a - b);
  }, [knownPlayers, teams]);

  function teamOfCommand(cmd: number): number {
    return teams.findIndex((t) => t.members.includes(cmd));
  }
  function assignToTeam(cmd: number, teamIdx: number) {
    setTeams(
      teams.map((t, i) => ({
        ...t,
        members: i === teamIdx
          ? Array.from(new Set([...t.members, cmd]))
          : t.members.filter((m) => m !== cmd)
      }))
    );
  }
  function unassignCommand(cmd: number) {
    setTeams(teams.map((t) => ({ ...t, members: t.members.filter((m) => m !== cmd) })));
  }
  function addTeam() {
    if (teams.length >= 10) return;
    setTeams([...teams, { name: `Team ${teams.length + 1}`, color: DEFAULT_COLORS[teams.length] || '#64748b', members: [] }]);
  }
  function removeTeam(idx: number) {
    if (teams.length <= 2) return;
    setTeams(teams.filter((_, i) => i !== idx));
  }
  function updateTeam(idx: number, patch: Partial<TeamDef>) {
    setTeams(teams.map((t, i) => (i === idx ? { ...t, ...patch } : t)));
  }

  const [pin, setPin] = useState('');
  const [msg, setMsg] = useState<string | null>(null);
  const [err, setErr] = useState<string | null>(null);
  const [startPending, setStartPending] = useState(p.startRequested);

  useEffect(() => setStartPending(p.startRequested), [p.startRequested]);

  function sendOverWs(payload: any): boolean {
    const sock = ws.current;
    if (!sock || sock.readyState !== 1) return false;
    sock.send(JSON.stringify(payload));
    return true;
  }

  async function saveSettings(e: React.FormEvent) {
    e.preventDefault();
    setMsg(null);
    setErr(null);

    const payload = {
      pin,
      mode,
      lobbySeconds: starttimer,
      matchSeconds: duration,
      zone1Points: zone1,
      zone2Points: zone2,
      zone3Points: zone3,
      teams: mode === 'team' ? teams : []
    };

    // Try WebSocket first; fallback to POST if not connected
    if (sendOverWs({ action: 'updateSettings', ...payload })) {
      // ACK comes back via WS handler; show optimistic message
      setMsg('Gespeichert (via WebSocket). ESP übernimmt beim nächsten Pull (~30 s).');
      return;
    }
    const r = await fetch(`/api/esp/${p.id}/settings`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    if (!r.ok) {
      const j = await r.json().catch(() => null);
      setErr(j?.error === 'invalid_pin' ? 'Ungültiger PIN.' : (j?.error || 'Serverfehler.'));
      return;
    }
    setMsg('Gespeichert. ESP übernimmt die Werte beim nächsten Pull (~30 s).');
  }

  async function startTimer() {
    setMsg(null);
    setErr(null);
    if (pin.length < 4) { setErr('Bitte PIN eingeben.'); return; }
    if (sendOverWs({ action: 'startMatch', pin })) {
      setStartPending(true);
      setMsg('Timer angefordert. ESP startet bei nächstem Pull (~30 s).');
      return;
    }
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
    setMsg('Timer angefordert. ESP startet bei nächstem Pull (~30 s).');
  }

  return (
    <div style={{ maxWidth: 720 }}>
      <section className="hmy-card">
        <div className="hmy-card__header">Match-Defaults</div>
        <div className="hmy-card__body">
          <p style={{ marginTop: 0, color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
            Live über WebSocket. Änderungen werden sofort gespeichert und der ESP holt sie beim nächsten Pull (~30 s).
          </p>
          <form onSubmit={saveSettings}>
            <div className="hmy-field">
              <label className="hmy-field__label" htmlFor="modus">Modus</label>
              <select
                id="modus"
                className="hmy-select"
                value={mode}
                onChange={(e) => setMode(e.target.value as Mode)}
              >
                <option value="free-for-all">Alle gegen alle</option>
                <option value="team">Teammodus</option>
              </select>
            </div>

            <div className="hmy-field">
              <label className="hmy-field__label">Starttimer (Sekunden Verteilphase)</label>
              <input className="hmy-input" type="number" min={5} max={600} value={starttimer} onChange={(e) => setStarttimer(Number(e.target.value))} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">Runden-Dauer (Sekunden)</label>
              <input className="hmy-input" type="number" min={30} max={3600} value={duration} onChange={(e) => setDuration(Number(e.target.value))} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">Punkte Zone1 (Brust)</label>
              <input className="hmy-input" type="number" min={1} max={100} value={zone1} onChange={(e) => setZone1(Number(e.target.value))} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">Punkte Zone2 (Schultern)</label>
              <input className="hmy-input" type="number" min={1} max={100} value={zone2} onChange={(e) => setZone2(Number(e.target.value))} />
            </div>
            <div className="hmy-field">
              <label className="hmy-field__label">Punkte Zone3 (Ruecken/Waffe)</label>
              <input className="hmy-input" type="number" min={1} max={100} value={zone3} onChange={(e) => setZone3(Number(e.target.value))} />
            </div>

            {mode === 'team' && (
              <fieldset style={{ border: '1px solid var(--hmy-color-border-default)', borderRadius: 'var(--hmy-radius-base)', padding: 'var(--hmy-spacing-3)', margin: 'var(--hmy-spacing-4) 0' }}>
                <legend style={{ padding: '0 0.5rem', fontWeight: 600 }}>Teams ({teams.length} / 10)</legend>
                {teams.map((t, i) => (
                  <div key={i} style={{ display: 'grid', gridTemplateColumns: '36px 1fr auto', gap: 'var(--hmy-spacing-3)', alignItems: 'center', marginBottom: 'var(--hmy-spacing-3)' }}>
                    <input
                      type="color"
                      value={t.color}
                      onChange={(e) => updateTeam(i, { color: e.target.value })}
                      aria-label={`Farbe ${t.name}`}
                      style={{ width: 36, height: 36, padding: 0, border: '1px solid var(--hmy-color-border-default)', borderRadius: 'var(--hmy-radius-base)', background: 'transparent', cursor: 'pointer' }}
                    />
                    <input
                      className="hmy-input"
                      value={t.name}
                      onChange={(e) => updateTeam(i, { name: e.target.value })}
                      placeholder="Team-Name"
                      maxLength={12}
                    />
                    <button
                      type="button"
                      className="hmy-btn hmy-btn--sm hmy-btn--danger"
                      onClick={() => removeTeam(i)}
                      disabled={teams.length <= 2}
                      aria-label={`${t.name} entfernen`}
                    >
                      ✕
                    </button>
                  </div>
                ))}
                <button type="button" className="hmy-btn hmy-btn--sm" onClick={addTeam} disabled={teams.length >= 10}>
                  + Team hinzufügen
                </button>

                <h4 style={{ marginTop: '1.5rem', marginBottom: '0.5rem' }}>Spielerzuordnung</h4>
                <p style={{ fontSize: 'var(--hmy-font-size-xs)', color: 'var(--hmy-color-text-muted)', margin: '0 0 var(--hmy-spacing-3)' }}>
                  {knownPlayers.length === 0 && teams.every((t) => t.members.length === 0)
                    ? 'Noch keine Clients verbunden. Sobald ein Player-ESP über ESP-NOW Daten sendet, erscheint er hier (Live via WebSocket).'
                    : `${knownPlayers.length} Client(s) gemeldet. Wähle pro Spieler ein Team.`}
                </p>

                {allCommands.length > 0 && (
                  <table>
                    <thead>
                      <tr>
                        <th>Client</th>
                        <th className="num">NEC</th>
                        <th>Team</th>
                      </tr>
                    </thead>
                    <tbody>
                      {allCommands.map((cmd) => {
                        const known = knownPlayers.find((kp) => kp.command === cmd);
                        const idx = teamOfCommand(cmd);
                        return (
                          <tr key={cmd}>
                            <td>
                              <code className="hmy-code">{known?.name ?? `PLAYER_${cmd}`}</code>
                              {!known && (
                                <span style={{ marginLeft: 8, fontSize: 'var(--hmy-font-size-xs)', color: 'var(--hmy-color-text-muted)' }}>
                                  (offline)
                                </span>
                              )}
                            </td>
                            <td className="num">{cmd}</td>
                            <td>
                              <select
                                className="hmy-select"
                                value={idx}
                                onChange={(e) => {
                                  const v = Number(e.target.value);
                                  if (v < 0) unassignCommand(cmd);
                                  else assignToTeam(cmd, v);
                                }}
                                style={
                                  idx >= 0
                                    ? { borderLeft: `4px solid ${teams[idx].color}`, paddingLeft: '0.5rem' }
                                    : {}
                                }
                              >
                                <option value={-1}>— Kein Team —</option>
                                {teams.map((t, i) => (
                                  <option key={i} value={i}>{t.name}</option>
                                ))}
                              </select>
                            </td>
                          </tr>
                        );
                      })}
                    </tbody>
                  </table>
                )}
              </fieldset>
            )}

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
        <div className="hmy-card__header">Match Beginnen</div>
        <div className="hmy-card__body">
          <p style={{ marginTop: 0, color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)' }}>
            Setzt ein Flag, das der ESP-Server beim nächsten Pull (~30 s) aufnimmt. Anschließend
            beginnt der Starttimer mit den oben gesetzten Werten.
          </p>
          {startPending && (
            <div className="hmy-alert hmy-alert--info">
              Timer-Start steht aus — ESP nimmt ihn beim nächsten Pull auf.
            </div>
          )}
          <button className="hmy-btn hmy-btn--primary" type="button" onClick={startTimer}>
            ▶ Timer starten
          </button>
        </div>
      </section>
    </div>
  );
}
