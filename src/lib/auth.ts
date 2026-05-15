import argon2 from 'argon2';

export async function hashPassword(plain: string): Promise<string> {
  return argon2.hash(plain, { type: argon2.argon2id, timeCost: 3, memoryCost: 2 ** 16 });
}

export async function verifyPassword(hash: string, plain: string): Promise<boolean> {
  try {
    return await argon2.verify(hash, plain);
  } catch {
    return false;
  }
}

export function validateUsername(u: string): string | null {
  if (!u || u.length < 3) return 'username_too_short';
  if (u.length > 32) return 'username_too_long';
  if (!/^[a-zA-Z0-9_.-]+$/.test(u)) return 'username_invalid_chars';
  return null;
}

export function validatePassword(p: string): string | null {
  if (!p || p.length < 8) return 'password_too_short';
  if (p.length > 128) return 'password_too_long';
  return null;
}
