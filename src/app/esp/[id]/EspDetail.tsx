'use client';

import Link from 'next/link';
import { useEffect, useMemo, useState } from 'react';
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

      {tab === 'overview' ? <Overview p={p} tm={tm} /> : <Settings p={p} />}
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

function Settings({ p }: { p: Props }) {
  const [mode, setMode] = useState<Mode>((p.mode === 'team' ? 'team' : 'free-for-all'));
  const [starttimer, setStarttimer] = useState(p.lobbySeconds);
  const [duration, setDuration] = useState(p.matchSeconds);

  const [teams, setTeams] = useState<TeamDef[]>(
    p.teams.length > 0
      ? p.teams
      : [
          { name: 'Rot', color: DEFAULT_COLORS[0], members: [] },
          { name: 'Blau', color: DEFAULT_COLORS[1], members: [] }
        ]
  );

  // Live-Refresh der bekannten Clients: alle 5 s neu pollen
  const [knownPlayers, setKnownPlayers] = useState<KnownPlayer[]>(p.knownPlayers);
  useEffect(() => {
    const t = setInterval(async () => {
      try {
        const r = await fetch(`/api/esp/${p.id}/players`, { cache: 'no-store' });
        if (r.ok) {
          const d = await r.json();
          setKnownPlayers(d.players || []);
        }
      } catch {}
    }, 5000);
    return () => clearInterval(t);
  }, [p.id]);

  // Plus alle Mitglieder, die in den Teams angegeben sind, aber (noch) nicht
  // als knownPlayer gemeldet wurden — damit eine Vorab-Konfig nicht verloren geht.
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
    if (teams.length >= 4) return;
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

  async function saveSettings(e: React.FormEvent) {
    e.preventDefault();
    setMsg(null);
    setErr(null);
    const body: any = { pin, mode, lobbySeconds: starttimer, matchSeconds: duration };
    body.teams = mode === 'team' ? teams : [];
    const r = await fetch(`/api/esp/${p.id}/settings`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
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
            Diese Werte werden vom ESP regelmäßig gepulled und für jedes neue Match verwendet.
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

            {mode === 'team' && (
              <fieldset style={{ border: '1px solid var(--hmy-color-border-default)', borderRadius: 'var(--hmy-radius-base)', padding: 'var(--hmy-spacing-3)', margin: 'var(--hmy-spacing-4) 0' }}>
                <legend style={{ padding: '0 0.5rem', fontWeight: 600 }}>Teams ({teams.length} / 4)</legend>
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
                <button type="button" className="hmy-btn hmy-btn--sm" onClick={addTeam} disabled={teams.length >= 4}>
                  + Team hinzufügen
                </button>

                <h4 style={{ marginTop: '1.5rem', marginBottom: '0.5rem' }}>Spielerzuordnung</h4>
                <p style={{ fontSize: 'var(--hmy-font-size-xs)', color: 'var(--hmy-color-text-muted)', margin: '0 0 var(--hmy-spacing-3)' }}>
                  {knownPlayers.length === 0 && teams.every((t) => t.members.length === 0)
                    ? 'Noch keine Clients verbunden. Sobald ein Player-ESP über ESP-NOW Daten sendet, erscheint er hier.'
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
