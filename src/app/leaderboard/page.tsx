import { getTranslations } from 'next-intl/server';
import { db } from '@/lib/db';

export const dynamic = 'force-dynamic';

export default async function LeaderboardPage() {
  const t = await getTranslations('leaderboard');

  const rows = await db.$queryRaw<
    Array<{ username: string; matches: bigint; hits: bigint; points: bigint }>
  >`
    SELECT u.username,
           COUNT(DISTINCT mp."matchId") AS matches,
           COALESCE(SUM(mp.hits), 0)::bigint AS hits,
           COALESCE(SUM(mp.points), 0)::bigint AS points
      FROM "User" u
      LEFT JOIN "MatchPlayer" mp ON mp."userId" = u.id
     GROUP BY u.id, u.username
     HAVING COUNT(DISTINCT mp."matchId") > 0
     ORDER BY points DESC, hits DESC
     LIMIT 100
  `;

  return (
    <>
      <h1>{t('title')}</h1>
      {rows.length === 0 ? (
        <p>{t('empty')}</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>{t('rank')}</th>
              <th>{t('player')}</th>
              <th>{t('matches')}</th>
              <th>{t('hits')}</th>
              <th>{t('points')}</th>
            </tr>
          </thead>
          <tbody>
            {rows.map((r, i) => (
              <tr key={r.username}>
                <td>{i + 1}</td>
                <td>{r.username}</td>
                <td>{r.matches.toString()}</td>
                <td>{r.hits.toString()}</td>
                <td>{r.points.toString()}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </>
  );
}
