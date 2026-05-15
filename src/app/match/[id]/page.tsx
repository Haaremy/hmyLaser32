import { notFound } from 'next/navigation';
import { db } from '@/lib/db';
import { MatchView } from './MatchView';

export const dynamic = 'force-dynamic';

export default async function MatchPage({ params }: { params: { id: string } }) {
  const match = await db.match.findUnique({
    where: { id: params.id },
    select: { id: true }
  });
  if (!match) notFound();
  return <MatchView matchId={match.id} />;
}
