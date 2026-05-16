import { Prisma } from '@prisma/client';
import { z } from 'zod';

/** Eine Team-Definition aus den Server-Settings. */
export const teamSchema = z.object({
  name: z.string().min(1).max(12),
  color: z.string().regex(/^#[0-9a-fA-F]{6}$/),
  members: z.array(z.number().int().min(1).max(254)).max(10)
});

export const teamsSchema = z.array(teamSchema).max(4);
export type TeamDef = z.infer<typeof teamSchema>;

export function parseTeams(raw: Prisma.JsonValue | null | undefined): TeamDef[] {
  if (!raw) return [];
  const r = teamsSchema.safeParse(raw);
  return r.success ? r.data : [];
}
