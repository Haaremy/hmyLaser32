import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';
import { ChampionsView } from './ChampionsView';

export const dynamic = 'force-dynamic';

export default async function ChampionsPage() {
  const t = await getTranslations('champions');

  const rows = await db.$queryRaw<
    Array<{
      username: string;
      matches: bigint;
      hits: bigint;
      deaths: bigint;
      points: bigint;
    }>
  >`
    SELECT u.username,
           COUNT(DISTINCT mp."matchId") AS matches,
           COALESCE(SUM(mp.hits),   0)::bigint AS hits,
           COALESCE(SUM(mp.deaths), 0)::bigint AS deaths,
           COALESCE(SUM(mp.points), 0)::bigint AS points
      FROM "User" u
      LEFT JOIN "MatchPlayer" mp ON mp."userId" = u.id
      LEFT JOIN "Match" m ON m.id = mp."matchId"
     WHERE m.id IS NULL OR m.status = 'finished'
     GROUP BY u.id, u.username
     HAVING COUNT(DISTINCT mp."matchId") > 0
     LIMIT 200
  `;

  const players = rows.map((r) => {
    const hits = Number(r.hits);
    const deaths = Number(r.deaths);
    return {
      username: r.username,
      matches: Number(r.matches),
      hits,
      deaths,
      points: Number(r.points),
      kd: deaths === 0 ? hits : Number((hits / deaths).toFixed(2))
    };
  });

  return (
    <>
      <h1>{t('title')}</h1>
      <p>{t('subtitle')}</p>
      {players.length === 0 ? (
        <div className="hmy-alert hmy-alert--info">{t('empty')}</div>
      ) : (
        <ChampionsView players={players} />
      )}
    </>
  );
}
