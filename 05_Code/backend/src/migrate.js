import { readdir, readFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createPool } from './db.js';

const migrationsDirectory = join(dirname(fileURLToPath(import.meta.url)), '..', 'db', 'migrations');

const pool = createPool();
const client = await pool.connect();

try {
  await client.query(`
    CREATE TABLE IF NOT EXISTS _schema_migrations (
      filename TEXT PRIMARY KEY,
      applied_at TIMESTAMPTZ NOT NULL DEFAULT now()
    )
  `);

  const migrationFiles = (await readdir(migrationsDirectory))
    .filter((filename) => filename.endsWith('.sql'))
    .sort();

  for (const filename of migrationFiles) {
    const applied = await client.query(
      'SELECT 1 FROM _schema_migrations WHERE filename = $1',
      [filename],
    );
    if (applied.rowCount > 0) {
      continue;
    }

    const sql = await readFile(join(migrationsDirectory, filename), 'utf8');
    await client.query('BEGIN');
    try {
      await client.query(sql);
      await client.query('INSERT INTO _schema_migrations (filename) VALUES ($1)', [filename]);
      await client.query('COMMIT');
      console.log(`Migracion aplicada: ${filename}`);
    } catch (error) {
      await client.query('ROLLBACK');
      throw error;
    }
  }
} finally {
  client.release();
  await pool.end();
}