import { NextResponse } from 'next/server';
import { z } from 'zod';
import { db } from '@/lib/db';
import { authenticateBridge } from '@/lib/bridge';

export const runtime = 'nodejs';

const schema = z.object({
  serverName: z.string().min(1).max(64).optional(),
  mode: z.string().min(1).max(32).optional(),
  durationSeconds: z.number().int().positive().max(86400).optional(),
  hitPoints: z.number().int().positive().max(100).optional(),
  zone1Points: z.number().int().positive().max(100).optional(),
  zone2Points: z.number().int().positive().max(100).optional(),
  zone3Points: z.number().int().positive().max(100).optional(),
  teams: z
    .array(z.object({ name: z.string().min(1).max(32), color: z.string().min(1).max(16) }))
    .max(10)
    .optional()
});

export async function POST(req: Request) {
  const server = await authenticateBridge(req.headers.get('authorization'));
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });

  let body: z.infer<typeof schema>;
  try {
    body = schema.parse(await req.json());
  } catch (e) {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }

  const match = await db.match.create({
    data: {
      serverId: server.id,
      startedAt: new Date(),
      durationSeconds: body.durationSeconds ?? null,
      mode: body.mode ?? null,
      status: 'active',
      settings: {
        hitPoints: body.zone1Points ?? body.hitPoints ?? 15,
        zone1Points: body.zone1Points ?? body.hitPoints ?? 15,
        zone2Points: body.zone2Points ?? 10,
        zone3Points: body.zone3Points ?? 5
      },
      teams: body.teams ? { create: body.teams.map((t) => ({ name: t.name, color: t.color })) } : undefined
    },
    include: { teams: true }
  });

  return NextResponse.json({ matchId: match.id, teams: match.teams });
}
