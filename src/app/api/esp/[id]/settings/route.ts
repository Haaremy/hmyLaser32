import { NextResponse } from 'next/server';
import { z } from 'zod';
import { Prisma } from '@prisma/client';
import { db } from '@/lib/db';
import { parseTeams, teamsSchema } from '@/lib/teams';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

async function authServer(req: Request, id: string, bodyPin?: string) {
  const auth = req.headers.get('authorization');
  let pin = bodyPin?.trim() || '';
  if (!pin && auth && auth.startsWith('Bearer ')) pin = auth.substring(7).trim();
  if (!pin) return null;
  const server = await db.espServer.findUnique({ where: { id } });
  if (!server) return null;
  return server.pin === pin ? server : null;
}

export async function GET(_req: Request, { params }: { params: { id: string } }) {
  const server = await db.espServer.findUnique({
    where: { id: params.id },
    select: {
      id: true, name: true, mode: true, lobbySeconds: true, matchSeconds: true,
      teams: true, startRequested: true, lastSeen: true, online: true
    }
  });
  if (!server) return NextResponse.json({ error: 'not_found' }, { status: 404 });
  return NextResponse.json({ ...server, teams: parseTeams(server.teams) });
}

const schema = z.object({
  pin: z.string().min(4).max(32),
  mode: z.enum(['free-for-all', 'team']).optional(),
  lobbySeconds: z.number().int().min(5).max(600).optional(),
  matchSeconds: z.number().int().min(30).max(3600).optional(),
  teams: teamsSchema.optional()
});

export async function POST(req: Request, { params }: { params: { id: string } }) {
  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch (e) {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
  const server = await authServer(req, params.id, body.pin);
  if (!server) return NextResponse.json({ error: 'invalid_pin' }, { status: 403 });

  const updated = await db.espServer.update({
    where: { id: server.id },
    data: {
      mode: body.mode ?? server.mode,
      lobbySeconds: body.lobbySeconds ?? server.lobbySeconds,
      matchSeconds: body.matchSeconds ?? server.matchSeconds,
      teams: body.teams !== undefined ? (body.teams as Prisma.InputJsonValue) : undefined
    },
    select: { id: true, mode: true, lobbySeconds: true, matchSeconds: true, teams: true }
  });

  // WS-Broadcast an Browser-Subscribers
  (globalThis as any).wsHub?.broadcastEsp?.(server.id, { type: 'invalidate' });

  return NextResponse.json({ ok: true, ...updated, teams: parseTeams(updated.teams) });
}
