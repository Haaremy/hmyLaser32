import { NextResponse } from 'next/server';
import { z } from 'zod';
import { authenticateBridge } from '@/lib/bridge';

export const runtime = 'nodejs';

const schema = z.object({
  matchId: z.string().min(1),
  shooterNfc: z.string().max(64).optional(),
  targetNfc: z.string().max(64).optional(),
  ts: z.number().int().optional()
});

export async function POST(req: Request) {
  const server = await authenticateBridge(req.headers.get('authorization'));
  if (!server) return NextResponse.json({ error: 'unauthorized' }, { status: 401 });
  try {
    const data = schema.parse(await req.json());
    // Live-only event: persistence happens on /match/end with aggregated stats.
    // Forward to WebSocket subscribers (see /api/bridge/ws/route.ts).
    return NextResponse.json({ ok: true, ts: data.ts ?? Date.now() });
  } catch {
    return NextResponse.json({ error: 'invalid_body' }, { status: 400 });
  }
}
