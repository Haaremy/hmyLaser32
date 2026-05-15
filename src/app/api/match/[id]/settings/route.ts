import { NextResponse } from 'next/server';
import { z } from 'zod';
import { Prisma } from '@prisma/client';
import { db } from '@/lib/db';
import { pinMatchesServer } from '@/lib/bridge';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

const schema = z.object({
  pin: z.string().min(4).max(32),
  settings: z.record(z.unknown())
});

/** GET — Settings lesen (öffentlich, keine PIN nötig — wer den Match sieht, sieht auch Settings). */
export async function GET(_req: Request, { params }: { params: { id: string } }) {
  const m = await db.match.findUnique({
    where: { id: params.id },
    select: { id: true, settings: true, mode: true, durationSeconds: true }
  });
  if (!m) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  return NextResponse.json({ settings: m.settings || {}, mode: m.mode, durationSeconds: m.durationSeconds });
}

/** POST — Settings ändern, PIN-gated. */
export async function POST(req: Request, { params }: { params: { id: string } }) {
  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const m = await db.match.findUnique({ where: { id: params.id }, select: { id: true, serverId: true } });
  if (!m) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  const ok = await pinMatchesServer(m.serverId, body.pin);
  if (!ok) return NextResponse.json({ error: 'invalid_pin' }, { status: 403 });

  const updated = await db.match.update({
    where: { id: m.id },
    data: { settings: body.settings as Prisma.InputJsonValue },
    select: { id: true, settings: true }
  });
  return NextResponse.json({ ok: true, settings: updated.settings });
}
