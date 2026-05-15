import { NextResponse } from 'next/server';
import { db } from '@/lib/db';
import { generateAccessCode } from '@/lib/bridge';
import { getSession } from '@/lib/session';

export const runtime = 'nodejs';

export async function GET() {
  const session = await getSession();
  if (session.role !== 'ADMIN') return NextResponse.json({ error: 'forbidden' }, { status: 403 });
  const servers = await db.espServer.findMany({
    orderBy: { createdAt: 'desc' },
    select: { id: true, name: true, lastSeen: true, accessCode: true, createdAt: true }
  });
  return NextResponse.json({ servers });
}

export async function POST(req: Request) {
  const session = await getSession();
  if (session.role !== 'ADMIN') return NextResponse.json({ error: 'forbidden' }, { status: 403 });
  const { name } = await req.json();
  if (typeof name !== 'string' || !name.trim()) {
    return NextResponse.json({ error: 'invalid_name' }, { status: 400 });
  }
  const accessCode = generateAccessCode();
  const server = await db.espServer.create({
    data: { name: name.trim(), accessCode, ownerId: session.userId || null },
    select: { id: true, name: true, accessCode: true }
  });
  return NextResponse.json({ server });
}
