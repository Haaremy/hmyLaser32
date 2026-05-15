import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { verifyPassword } from '@/lib/auth';
import { getSession } from '@/lib/session';

export const runtime = 'nodejs';

export async function POST(req: Request) {
  try {
    const { username, password } = await req.json();
    if (typeof username !== 'string' || typeof password !== 'string') {
      return NextResponse.json({ error: 'invalid_credentials' }, { status: 400 });
    }
    const user = await db.user.findUnique({ where: { username } });
    if (!user) return NextResponse.json({ error: 'invalid_credentials' }, { status: 401 });
    const ok = await verifyPassword(user.passwordHash, password);
    if (!ok) return NextResponse.json({ error: 'invalid_credentials' }, { status: 401 });

    const session = await getSession();
    session.userId = user.id;
    session.username = user.username;
    session.role = user.role;
    await session.save();

    return NextResponse.json({ ok: true });
  } catch (e) {
    console.error('login error', e);
    return NextResponse.json({ error: 'server_error' }, { status: 500 });
  }
}
