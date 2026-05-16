import { Prisma } from '@prisma/client';
import { z } from 'zod';

export const knownPlayerSchema = z.object({
  name: z.string().min(1).max(12),
  command: z.number().int().min(1).max(254),
  playerId: z.number().int().nonnegative(),
  points: z.number().int().optional().default(0),
  lastSeenSec: z.number().int().nonnegative().optional().default(0)
});

export const knownPlayersSchema = z.array(knownPlayerSchema).max(20);
export type KnownPlayer = z.infer<typeof knownPlayerSchema>;

export function parseKnownPlayers(raw: Prisma.JsonValue | null | undefined): KnownPlayer[] {
  if (!raw) return [];
  const r = knownPlayersSchema.safeParse(raw);
  return r.success ? r.data : [];
}
