import { NextResponse } from 'next/server';
import { z } from 'zod';
import { db } from '@/lib/db';

export const runtime = 'nodejs';

const schema = z.object({
  name: z.string().min(2).max(48),
  pin: z.string().min(4).max(32)
});

/**
 * ESP-Server-Self-Registration.
 * Der ESP startet, generiert lokal name+pin, sendet sie via POST.
 * - Existiert (name, pin)-Paar bereits → 200 (Re-Register-OK, lastSeen aktualisiert)
 * - Existiert nur name, anderer pin → 409 (Konflikt, ESP soll neuen Namen wählen)
 * - Existiert nur pin, anderer name → 409 (Konflikt, ESP soll neuen PIN wählen)
 * - Beide unbekannt → 201 (neu angelegt)
 */
export async function POST(req: Request) {
  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const { name, pin } = body;

  const byName = await db.espServer.findUnique({ where: { name } });
  const byPin = await db.espServer.findUnique({ where: { pin } });

  if (byName && byPin && byName.id === byPin.id) {
    await db.espServer.update({ where: { id: byName.id }, data: { lastSeen: new Date(), online: true } });
    return NextResponse.json({ ok: true, status: 'rebound', id: byName.id, name: byName.name }, { status: 200 });
  }
  if (byName && (!byPin || byName.id !== byPin.id)) {
    return NextResponse.json({ error: 'name_taken' }, { status: 409 });
  }
  if (byPin && (!byName || byPin.id !== byName.id)) {
    return NextResponse.json({ error: 'pin_taken' }, { status: 409 });
  }

  const server = await db.espServer.create({
    data: { name, pin, lastSeen: new Date(), online: true },
    select: { id: true, name: true }
  });
  return NextResponse.json({ ok: true, status: 'created', id: server.id, name: server.name }, { status: 201 });
}

/** Heartbeat endpoint (lightweight). ESP polls this every ~30s when no match is active. */
export async function GET(req: Request) {
  const auth = req.headers.get('authorization');
  if (!auth || !auth.startsWith('Bearer ')) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  const pin = auth.substring(7).trim();
  const server = await db.espServer.findUnique({ where: { pin }, select: { id: true, name: true } });
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  await db.espServer.update({ where: { id: server.id }, data: { lastSeen: new Date(), online: true } });
  return NextResponse.json({ ok: true, name: server.name });
}
