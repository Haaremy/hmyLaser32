'use client';

import { useMemo, useState } from 'react';
import { useTranslations } from 'next-intl';

export type ChampionPlayer = {
  username: string;
  matches: number;
  hits: number;
  deaths: number;
  points: number;
  kd: number;
};

type FilterKey = 'kills' | 'matches' | 'kd' | 'overall';

export function ChampionsView({ players }: { players: ChampionPlayer[] }) {
  const t = useTranslations('champions');
  const [filter, setFilter] = useState<FilterKey>('overall');

  const sorted = useMemo(() => {
    const copy = [...players];
    switch (filter) {
      case 'kills':
        return copy.sort((a, b) => b.hits - a.hits || b.points - a.points);
      case 'matches':
        return copy.sort((a, b) => b.matches - a.matches || b.points - a.points);
      case 'kd':
        return copy.sort((a, b) => b.kd - a.kd || b.hits - a.hits);
      case 'overall':
      default:
        return copy.sort((a, b) => b.points - a.points || b.hits - a.hits);
    }
  }, [players, filter]);

  return (
    <>
      <div className="hmy-lt-filterbar" role="tablist" aria-label="Filter">
        <button
          role="tab"
          aria-selected={filter === 'overall'}
          className={`hmy-btn ${filter === 'overall' ? 'is-active' : 'hmy-btn--secondary'}`}
          onClick={() => setFilter('overall')}
        >
          🏆 {t('filter_overall')}
        </button>
        <button
          role="tab"
          aria-selected={filter === 'kills'}
          className={`hmy-btn ${filter === 'kills' ? 'is-active' : 'hmy-btn--secondary'}`}
          onClick={() => setFilter('kills')}
        >
          🎯 {t('filter_kills')}
        </button>
        <button
          role="tab"
          aria-selected={filter === 'matches'}
          className={`hmy-btn ${filter === 'matches' ? 'is-active' : 'hmy-btn--secondary'}`}
          onClick={() => setFilter('matches')}
        >
          📅 {t('filter_matches')}
        </button>
        <button
          role="tab"
          aria-selected={filter === 'kd'}
          className={`hmy-btn ${filter === 'kd' ? 'is-active' : 'hmy-btn--secondary'}`}
          onClick={() => setFilter('kd')}
        >
          ⚔ {t('filter_kd')}
        </button>
      </div>

      <div className="hmy-lt-lb-row is-head">
        <div>{t('rank')}</div>
        <div>{t('player')}</div>
        <div style={{ textAlign: 'right' }}>{t('matches')}</div>
        <div style={{ textAlign: 'right' }}>{t('kd')}</div>
        <div style={{ textAlign: 'right' }}>{t('hits')}</div>
        <div style={{ textAlign: 'right' }}>{t('points')}</div>
      </div>
      {sorted.map((p, i) => (
        <div className="hmy-lt-lb-row" key={p.username}>
          <div className={`hmy-lt-rank ${i === 0 ? 'is-gold' : i === 1 ? 'is-silver' : i === 2 ? 'is-bronze' : ''}`}>
            {i + 1}
          </div>
          <div className="hmy-lt-lb-name">{p.username}</div>
          <div className="hmy-lt-num">{p.matches}</div>
          <div className="hmy-lt-num">{p.kd}</div>
          <div className="hmy-lt-num">{p.hits}</div>
          <div className="hmy-lt-num hmy-lt-points">{p.points}</div>
        </div>
      ))}
    </>
  );
}
