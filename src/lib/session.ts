import { getIronSession, SessionOptions } from 'iron-session';
import { cookies } from 'next/headers';

export type SessionData = {
  userId?: string;
  username?: string;
  role?: 'USER' | 'ADMIN';
};

const sessionPassword = process.env.SESSION_SECRET || '';
if (sessionPassword.length < 32 && process.env.NODE_ENV === 'production') {
  throw new Error('SESSION_SECRET must be at least 32 characters');
}

export const sessionOptions: SessionOptions = {
  password: sessionPassword.padEnd(32, '_'),
  cookieName: 'hmylaser_session',
  cookieOptions: {
    secure: process.env.NODE_ENV === 'production',
    httpOnly: true,
    sameSite: 'lax',
    path: '/'
  }
};

export function getSession() {
  return getIronSession<SessionData>(cookies(), sessionOptions);
}

export async function requireUser() {
  const session = await getSession();
  if (!session.userId) {
    throw new Error('UNAUTHENTICATED');
  }
  return session;
}

export async function requireAdmin() {
  const session = await requireUser();
  if (session.role !== 'ADMIN') {
    throw new Error('FORBIDDEN');
  }
  return session;
}
