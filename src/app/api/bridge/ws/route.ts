import { NextResponse } from 'next/server';

export const runtime = 'nodejs';

// Next.js App Router does not natively expose the upgrade handshake.
// The WebSocket server is started by a custom Node entrypoint (server.mjs)
// which intercepts /api/bridge/ws upgrades. This route only exists so that
// fetches without Upgrade header receive a helpful hint.
export async function GET() {
  return NextResponse.json(
    {
      hint: 'Connect via WebSocket: wss://laser32.haaremy.de/api/bridge/ws',
      auth: 'Send `Authorization: Bearer <accessCode>` as a Sec-WebSocket-Protocol header or as ?token= query parameter'
    },
    { status: 426 }
  );
}
