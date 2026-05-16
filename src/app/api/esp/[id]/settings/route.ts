import { NextResponse } from 'next/server';
import { z } from 'zod';
import { db } from '@/lib/db';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

// PIN-Auth: entweder Bearer oder body.pin
async function authServer(req: Request, id: string, bodyPin?: string) {
  const auth = req.headers.get('authorization');
  let pin = bodyPin?.trim() || '';
  if (!pin && auth && auth.startsWith('Bearer ')) pin = auth.substring(7).trim();
  if (!pin) return null;
  const server = await db.espServer.findUnique({ where: { id } });
  if (!server) return null;
  return server.pin === pin ? server : null;
}

// GET — Settings lesen (öffentlich; jeder kann die Defaults sehen)
export async function GET(_req: Request, { params }: { params: { id: string } }) {
  const server = await db.espServer.findUnique({
    where: { id: params.id },
    select: {
      id: true, name: true, mode: true, lobbySeconds: true, matchSeconds: true,
      startRequested: true, lastSeen: true, online: true
    }
  });
  if (!server) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  return NextResponse.json(server);
}

const schema = z.object({
  pin: z.string().min(4).max(32),
  mode: z.string().min(1).max(23).optional(),
  lobbySeconds: z.number().int().min(5).max(600).optional(),
  matchSeconds: z.number().int().min(30).max(3600).optional()
});

// POST — Settings ändern (PIN-gated)
export async function POST(req: Request, { params }: { params: { id: string } }) {
  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const server = await authServer(req, params.id, body.pin);
  if (!server) return NextResponse.json({ error: 'invalid_pin' }, { status: 403 });

  const updated = await db.espServer.update({
    where: { id: server.id },
    data: {
      mode: body.mode ?? server.mode,
      lobbySeconds: body.lobbySeconds ?? server.lobbySeconds,
      matchSeconds: body.matchSeconds ?? server.matchSeconds
    },
    select: { id: true, mode: true, lobbySeconds: true, matchSeconds: true }
  });
  return NextResponse.json({ ok: true, ...updated });
}
