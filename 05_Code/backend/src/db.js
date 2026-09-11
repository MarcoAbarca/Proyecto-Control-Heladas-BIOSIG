import pg from 'pg';
import { config, requireDatabaseUrl } from './config.js';

const { Pool } = pg;

export function createPool() {
  return new Pool({
    connectionString: requireDatabaseUrl(),
    ssl: config.databaseSsl
      ? { rejectUnauthorized: config.databaseRejectUnauthorized }
      : false,
    max: 5,
    idleTimeoutMillis: 30_000,
    connectionTimeoutMillis: 10_000,
  });
}