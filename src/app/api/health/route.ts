import { NextResponse } from 'next/server';
import { db } from '@/lib/db';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

const startedAt = Date.now();

export async function GET() {
  const deps: Record<string, string> = {};
  let status = 'ok';
  try {
    await db.$queryRaw`SELECT 1`;
    deps.db = 'ok';
  } catch (e) {
    deps.db = 'fail';
    status = 'degraded';
  }
  return NextResponse.json(
    {
      status,
      app: 'hmyLaser32',
      version: process.env.npm_package_version || '0.1.0',
      uptime_s: Math.floor((Date.now() - startedAt) / 1000),
      deps,
      ts: new Date().toISOString()
    },
    { status: status === 'ok' ? 200 : 503 }
  );
}
