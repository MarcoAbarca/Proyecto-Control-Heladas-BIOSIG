import { createServer as createHttpServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { fileURLToPath } from 'node:url';
import { config } from './config.js';
import { createPool } from './db.js';
import { ingestTtnUplink } from './ingest.js';
import { evaluateTelemetryAlerts } from './alert_evaluator.js';

const publicDirectory = join(fileURLToPath(new URL('../public/', import.meta.url)));
const contentTypes = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
};

function sendJson(response, statusCode, body) {
  response.writeHead(statusCode, {
    'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store',
  });
  response.end(JSON.stringify(body));
}

function parseNodeId(value) {
  const nodeId = Number.parseInt(value, 10);
  if (!Number.isInteger(nodeId) || nodeId < 0 || nodeId > 255) {
    return null;
  }
  return nodeId;
}

function parseAlertSettings(body) {
  const settings = {
    frostEnabled: Boolean(body.frostEnabled),
    frostThresholdC: Number(body.frostThresholdC),
    heatEnabled: Boolean(body.heatEnabled),
    heatThresholdC: Number(body.heatThresholdC),
    sensorEnabled: Boolean(body.sensorEnabled),
    connectivityEnabled: Boolean(body.connectivityEnabled),
    telegramEnabled: Boolean(body.telegramEnabled),
  };
  if (!Number.isFinite(settings.frostThresholdC) || settings.frostThresholdC < -80 || settings.frostThresholdC > 80 ||
      !Number.isFinite(settings.heatThresholdC) || settings.heatThresholdC < -80 || settings.heatThresholdC > 100) {
    throw new RangeError('Umbrales de alerta fuera de rango');
  }
  return settings;
}

async function serveStatic(pathname, response) {
  const relativePath = pathname === '/' ? 'index.html' : pathname.slice(1);
  const filePath = normalize(join(publicDirectory, relativePath));
  if (!filePath.startsWith(publicDirectory)) {
    return false;
  }
  try {
    const body = await readFile(filePath);
    response.writeHead(200, {
      'Content-Type': contentTypes[extname(filePath)] ?? 'application/octet-stream',
      'Cache-Control': 'no-cache',
    });
    response.end(body);
    return true;
  } catch (error) {
    if (error.code !== 'ENOENT') throw error;
    return false;
  }
}

async function readJsonBody(request) {
  const chunks = [];
  let size = 0;
  for await (const chunk of request) {
    size += chunk.length;
    if (size > 64 * 1024) {
      throw new RangeError('Body demasiado grande');
    }
    chunks.push(chunk);
  }
  return JSON.parse(Buffer.concat(chunks).toString('utf8'));
}

export function createApiServer(pool) {
  return createHttpServer(async (request, response) => {
    const url = new URL(request.url ?? '/', `http://${request.headers.host ?? 'localhost'}`);
    response.setHeader('Access-Control-Allow-Origin', '*');

    try {
      if (request.method !== 'GET') {
        if (request.method === 'POST' && url.pathname === '/api/ttn/uplink') {
          const body = await readJsonBody(request);
          const result = await ingestTtnUplink(pool, body);
          
          if (result && result.measurement) {
            await evaluateTelemetryAlerts(pool, result.measurement);
          }

          sendJson(response, result.status === 'duplicate' ? 200 : 201, result);
          return;
        }

        // Endpoint POST para guardar un comentario de usuario/agrónomo en una medición
        const commentMatch = url.pathname.match(/^\/api\/telemetry\/(\d+)\/comment$/);
        if (request.method === 'POST' && commentMatch) {
          const telemetryId = Number.parseInt(commentMatch[1], 10);
          const body = await readJsonBody(request);

          await pool.query(`
            UPDATE telemetry_measurements 
            SET user_comment = $1 
            WHERE id = $2
          `, [body.comment ?? '', telemetryId]);

          sendJson(response, 200, { status: 'ok', telemetryId, comment: body.comment });
          return;
        }

        const alertMatch = url.pathname.match(/^\/api\/nodes\/(\d+)\/alert-config$/);
        if (request.method === 'PUT' && alertMatch) {
          const nodeId = parseNodeId(alertMatch[1]);
          const settings = parseAlertSettings(await readJsonBody(request));
          if (nodeId === null) {
            sendJson(response, 400, { error: 'nodeId invalido' });
            return;
          }
          await pool.query(`
            INSERT INTO nodes (node_id, display_name)
            VALUES ($1, $2)
            ON CONFLICT (node_id) DO NOTHING
          `, [nodeId, `Nodo ${nodeId}`]);
          
          const result = await pool.query(`
            INSERT INTO node_alert_settings (
              node_id, frost_enabled, frost_threshold_c, heat_enabled, heat_threshold_c,
              sensor_enabled, connectivity_enabled, telegram_enabled
            ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
            ON CONFLICT (node_id) DO UPDATE SET
              frost_enabled = EXCLUDED.frost_enabled,
              frost_threshold_c = EXCLUDED.frost_threshold_c,
              heat_enabled = EXCLUDED.heat_enabled,
              heat_threshold_c = EXCLUDED.heat_threshold_c,
              sensor_enabled = EXCLUDED.sensor_enabled,
              connectivity_enabled = EXCLUDED.connectivity_enabled,
              telegram_enabled = EXCLUDED.telegram_enabled,
              updated_at = now()
            RETURNING node_id AS "nodeId", frost_enabled AS "frostEnabled",
              frost_threshold_c AS "frostThresholdC", heat_enabled AS "heatEnabled",
              heat_threshold_c AS "heatThresholdC", sensor_enabled AS "sensorEnabled",
              connectivity_enabled AS "connectivityEnabled", telegram_enabled AS "telegramEnabled"
          `, [nodeId, settings.frostEnabled, settings.frostThresholdC, settings.heatEnabled,
            settings.heatThresholdC, settings.sensorEnabled, settings.connectivityEnabled,
            settings.telegramEnabled]);
          sendJson(response, 200, result.rows[0]);
          return;
        }
        sendJson(response, 405, { error: 'Metodo no permitido' });
        return;
      }

      if (url.pathname === '/api/health') {
        await pool.query('SELECT 1');
        sendJson(response, 200, { status: 'ok', database: 'ok' });
        return;
      }

      if (url.pathname === '/api/nodes') {
        const result = await pool.query(`
          SELECT n.node_id AS "nodeId", n.display_name AS "displayName", n.enabled,
                 s.last_sequence_number AS "lastSequenceNumber",
                 s.last_received_at AS "lastReceivedAt",
                 s.connection_state AS "connectionState",
                 s.last_status_flags AS "lastStatusFlags",
                 s.quality_metadata AS "qualityMetadata"
          FROM nodes n
          LEFT JOIN node_current_state s ON s.node_id = n.node_id
          WHERE n.enabled = TRUE
          ORDER BY n.node_id
        `);
        sendJson(response, 200, result.rows);
        return;
      }

      const alertMatch = url.pathname.match(/^\/api\/nodes\/(\d+)\/alert-config$/);
      if (alertMatch) {
        const nodeId = parseNodeId(alertMatch[1]);
        if (nodeId === null) {
          sendJson(response, 400, { error: 'nodeId invalido' });
          return;
        }
        const result = await pool.query(`
          SELECT node_id AS "nodeId", frost_enabled AS "frostEnabled",
            frost_threshold_c AS "frostThresholdC", heat_enabled AS "heatEnabled",
            heat_threshold_c AS "heatThresholdC", sensor_enabled AS "sensorEnabled",
            connectivity_enabled AS "connectivityEnabled", telegram_enabled AS "telegramEnabled"
          FROM node_alert_settings WHERE node_id = $1
        `, [nodeId]);
        sendJson(response, 200, result.rows[0] ?? {
          nodeId, frostEnabled: true, frostThresholdC: 0,
          heatEnabled: false, heatThresholdC: 35, sensorEnabled: true,
          connectivityEnabled: true, telegramEnabled: false,
        });
        return;
      }

      // Endpoint extendido para consultar telemetría con filtro de fechas o histórico por nodo
      if (url.pathname === '/api/telemetry/query') {
        const nodeId = url.searchParams.get('nodeId');
        const startDate = url.searchParams.get('startDate');
        const endDate = url.searchParams.get('endDate');

        let query = `
          SELECT id, node_id AS "nodeId", sequence_number AS "sequenceNumber",
                 received_at AS "receivedAt",
                 temp_canopy_top_c AS "tempCanopyTopC",
                 temp_canopy_mid_c AS "tempCanopyMidC",
                 temp_canopy_low_c AS "tempCanopyLowC",
                 temp_base_c AS "tempBaseC",
                 hum_base_pct AS "humBasePct",
                 pressure_hpa AS "pressureHpa",
                 soil_moisture_pct AS "soilMoisturePct",
                 temp_soil_c AS "tempSoilC",
                 user_comment AS "userComment"
          FROM telemetry_measurements
          WHERE 1=1
        `;
        const params = [];

        if (nodeId && nodeId !== 'all') {
          params.push(parseNodeId(nodeId));
          query += ` AND node_id = $${params.length}`;
        }
        if (startDate) {
          params.push(startDate);
          query += ` AND received_at >= $${params.length}::timestamp`;
        }
        if (endDate) {
          params.push(endDate);
          query += ` AND received_at <= $${params.length}::timestamp + interval '1 day'`;
        }

        query += ` ORDER BY received_at DESC LIMIT 500`;

        const result = await pool.query(query, params);
        sendJson(response, 200, result.rows);
        return;
      }

      const nodeMatch = url.pathname.match(/^\/api\/nodes\/(\d+)\/history$/);
      if (nodeMatch) {
        const nodeId = parseNodeId(nodeMatch[1]);
        const requestedHours = Number.parseInt(url.searchParams.get('hours') ?? '24', 10);

        if (nodeId === null) {
          sendJson(response, 400, { error: 'nodeId invalido' });
          return;
        }

        const limit = requestedHours <= 1 ? 6 : requestedHours <= 6 ? 18 : requestedHours <= 24 ? 50 : 120;

        const result = await pool.query(`
          SELECT * FROM (
            SELECT id, 
                   node_id AS "nodeId", node_id,
                   sequence_number AS "sequenceNumber", sequence_number,
                   received_at AS "receivedAt", received_at,
                   temp_canopy_top_c AS "tempCanopyTopC", temp_canopy_top_c,
                   temp_canopy_mid_c AS "tempCanopyMidC", temp_canopy_mid_c,
                   temp_canopy_low_c AS "tempCanopyLowC", temp_canopy_low_c,
                   temp_base_c AS "tempBaseC", temp_base_c,
                   hum_base_pct AS "humBasePct", hum_base_pct,
                   pressure_hpa AS "pressureHpa", pressure_hpa,
                   soil_moisture_pct AS "soilMoisturePct", soil_moisture_pct,
                   temp_soil_c AS "tempSoilC", temp_soil_c,
                   status_flags AS "statusFlags", status_flags,
                   quality_metadata AS "qualityMetadata", quality_metadata,
                   user_comment AS "userComment"
            FROM telemetry_measurements
            WHERE node_id = $1
            ORDER BY received_at DESC
            LIMIT $2
          ) sub
          ORDER BY "receivedAt" ASC
        `, [nodeId, limit]);

        sendJson(response, 200, result.rows);
        return;
      }

      if (url.pathname === '/api/alerts') {
        const result = await pool.query(`
          SELECT id, node_id AS "nodeId", alert_type AS "alertType", severity,
                 state, measurement_id AS "measurementId", message, value,
                 started_at AS "startedAt", resolved_at AS "resolvedAt",
                 details
          FROM alert_events
          ORDER BY started_at DESC
          LIMIT 100
        `);
        sendJson(response, 200, result.rows);
        return;
      }

      if (await serveStatic(url.pathname, response)) {
        return;
      }

      sendJson(response, 404, { error: 'Ruta no encontrada' });
    } catch (error) {
      console.error('Error en API:', error.message);
      sendJson(response, 503, { error: 'Servicio de datos no disponible' });
    }
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const pool = createPool();
  const server = createApiServer(pool);
  server.listen(config.port, () => {
    console.log(`API escuchando en http://localhost:${config.port}`);
  });
  const shutdown = async () => {
    server.close();
    await pool.end();
  };
  process.once('SIGINT', shutdown);
  process.once('SIGTERM', shutdown);
}