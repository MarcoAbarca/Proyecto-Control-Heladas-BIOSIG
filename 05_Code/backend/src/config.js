const port = Number.parseInt(process.env.PORT ?? '8080', 10);

if (!Number.isInteger(port) || port < 1 || port > 65535) {
  throw new Error('PORT debe ser un entero entre 1 y 65535');
}

export const config = Object.freeze({
  port,
  databaseUrl: process.env.DATABASE_URL ?? '',
  databaseSsl: process.env.PGSSL_DISABLE !== 'true',
  databaseRejectUnauthorized: process.env.PGSSL_REJECT_UNAUTHORIZED !== 'false',
  maxHistoryHours: Number.parseInt(process.env.MAX_HISTORY_HOURS ?? '168', 10),
});

export function requireDatabaseUrl() {
  if (!config.databaseUrl) {
    throw new Error('DATABASE_URL no esta configurada');
  }

  return config.databaseUrl;
}