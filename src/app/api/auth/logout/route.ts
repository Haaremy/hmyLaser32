import { NextResponse } from 'next/server';
import { getSession } from '@/lib/session';

export const runtime = 'nodejs';

async function handle(req: Request) {
  const session = await getSession();
  session.destroy();
  const url = new URL(req.url);
  return NextResponse.redirect(new URL('/account', url.origin));
}

export const POST = handle;
export const GET = handle;
