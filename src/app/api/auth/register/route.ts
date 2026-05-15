import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { hashPassword, validatePassword, validateUsername } from '@/lib/auth';
import { getSession } from '@/lib/session';

export const runtime = 'nodejs';

export async function POST(req: Request) {
  try {
    const { username, password } = await req.json();
    const uErr = validateUsername(username);
    if (uErr) return NextResponse.json({ error: uErr }, { status: 400 });
    const pErr = validatePassword(password);
    if (pErr) return NextResponse.json({ error: pErr }, { status: 400 });

    const existing = await db.user.findUnique({ where: { username } });
    if (existing) return NextResponse.json({ error: 'username_taken' }, { status: 409 });

    const passwordHash = await hashPassword(password);
    const user = await db.user.create({
      data: { username, passwordHash },
      select: { id: true, username: true, role: true, nfcToken: true }
    });

    const session = await getSession();
    session.userId = user.id;
    session.username = user.username;
    session.role = user.role;
    await session.save();

    return NextResponse.json({ ok: true, user });
  } catch (e) {
    console.error('register error', e);
    return NextResponse.json({ error: 'server_error' }, { status: 500 });
  }
}
