import { NextResponse } from 'next/server';
import { z } from 'zod';
import { db } from '@/lib/db';

export const runtime = 'nodejs';

const schema = z.object({ pin: z.string().min(4).max(32) });

export async function POST(req: Request, { params }: { params: { id: string } }) {
  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const server = await db.espServer.findUnique({ where: { id: params.id } });
  if (!server) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  if (server.pin !== body.pin.trim()) {
    return NextResponse.json({ error: 'invalid_pin' }, { status: 403 });
  }
  await db.espServer.update({
    where: { id: server.id },
    data: { startRequested: true }
  });
  (globalThis as any).wsHub?.broadcastEsp?.(server.id, { type: 'invalidate' });
  return NextResponse.json({ ok: true });
}
