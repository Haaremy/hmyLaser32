import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { getSession } from '@/lib/session';

export const runtime = 'nodejs';

export async function GET() {
  const session = await getSession();
  if (!session.userId) return NextResponse.json({ error: 'unauthenticated' }, { status: 401 });
  const user = await db.user.findUnique({
    where: { id: session.userId },
    select: { id: true, username: true, role: true, nfcToken: true, createdAt: true }
  });
  if (!user) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  return NextResponse.json({ user });
}
