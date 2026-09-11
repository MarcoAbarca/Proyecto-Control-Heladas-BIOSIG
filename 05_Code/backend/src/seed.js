import { createPool } from './db.js';

const pool = createPool();

async function seed() {
  try {
    console.log('🌱 Forzando regeneración de historial térmico en tiempo real...');

    // 1. Asegurar la existencia del Nodo 1 (nodo-receptor-curico-01)
    await pool.query(`
      INSERT INTO nodes (node_id, display_name, enabled, communication_timeout_seconds)
      VALUES (1, 'Nodo 1 - Curicó Fundo Demo', true, 1800)
      ON CONFLICT (node_id) DO UPDATE 
      SET display_name = EXCLUDED.display_name;
    `);

    // 2. Insertar configuraciones iniciales de alerta para el Nodo 1
    await pool.query(`
      INSERT INTO node_alert_settings (node_id, frost_enabled, frost_threshold_c, heat_enabled, heat_threshold_c, telegram_enabled)
      VALUES (1, true, 2.0, true, 25.0, true)
      ON CONFLICT (node_id) DO NOTHING;
    `);

    // 3. Generar mediciones históricas simuladas relativas a HORA ACTUAL EXACTA
    const now = new Date();
    let lastInsertedId = null;
    let lastSeq = 800;
    const defaultFlags = 0x003F;

    for (let i = 24; i >= 0; i--) {
      // Puntos generados hacia atrás desde HORA ACTUAL en intervalos de 15 minutos
      const timestamp = new Date(now.getTime() - i * 15 * 60 * 1000);
      const seq = 800 - i; // Secuencias 776 - 800
      
      // Simulación de caída de temperatura continua
      const tempBase = parseFloat((2.0 - (24 - i) * 0.20).toFixed(2));
      const tempCanopyTop = parseFloat((tempBase + 0.50).toFixed(2));
      const tempCanopyMid = parseFloat((tempBase + 0.20).toFixed(2));
      const tempCanopyLow = parseFloat((tempBase - 0.60).toFixed(2));

      const res = await pool.query(`
        INSERT INTO telemetry_measurements (
          node_id, sequence_number, received_at,
          temp_canopy_top_c, temp_canopy_mid_c, temp_canopy_low_c,
          temp_base_c, hum_base_pct, pressure_hpa,
          soil_moisture_pct, temp_soil_c, status_flags, raw_payload
        ) VALUES (
          $1, $2, $3,
          $4, $5, $6,
          $7, $8, $9,
          $10, $11, $12, $13
        )
        ON CONFLICT (node_id, sequence_number) DO UPDATE
        SET received_at = EXCLUDED.received_at,
            temp_canopy_top_c = EXCLUDED.temp_canopy_top_c,
            temp_canopy_mid_c = EXCLUDED.temp_canopy_mid_c,
            temp_canopy_low_c = EXCLUDED.temp_canopy_low_c,
            temp_base_c = EXCLUDED.temp_base_c,
            hum_base_pct = EXCLUDED.hum_base_pct,
            pressure_hpa = EXCLUDED.pressure_hpa,
            soil_moisture_pct = EXCLUDED.soil_moisture_pct,
            temp_soil_c = EXCLUDED.temp_soil_c,
            status_flags = EXCLUDED.status_flags
        RETURNING id;
      `, [
        1, seq, timestamp,
        tempCanopyTop, tempCanopyMid, tempCanopyLow,
        tempBase, 88.50, 1013.20,
        45.00, 8.20, defaultFlags, '\\x00'
      ]);

      if (i === 0 && res.rows.length > 0) {
        lastInsertedId = res.rows[0].id;
        lastSeq = seq;
      }
    }

    if (!lastInsertedId) {
      const lastRow = await pool.query(`
        SELECT id FROM telemetry_measurements 
        WHERE node_id = 1 ORDER BY received_at DESC LIMIT 1;
      `);
      if (lastRow.rows.length > 0) {
        lastInsertedId = lastRow.rows[0].id;
      }
    }

    // 4. Actualizar el estado actual del nodo
    if (lastInsertedId) {
      await pool.query(`
        INSERT INTO node_current_state (
          node_id, measurement_id, last_sequence_number, last_received_at, connection_state, last_status_flags, quality_metadata, updated_at
        ) VALUES (
          1, $1, $2, $3, 'online', $4, '{}'::jsonb, $3
        )
        ON CONFLICT (node_id) DO UPDATE
        SET measurement_id = EXCLUDED.measurement_id,
            last_sequence_number = EXCLUDED.last_sequence_number,
            last_received_at = EXCLUDED.last_received_at,
            connection_state = 'online',
            last_status_flags = EXCLUDED.last_status_flags,
            quality_metadata = EXCLUDED.quality_metadata,
            updated_at = EXCLUDED.updated_at;
      `, [lastInsertedId, lastSeq, now, defaultFlags]);
    }

    console.log('✅ Base de datos poblada exitosamente en el rango UTC actual.');
  } catch (err) {
    console.error('❌ Error en el seed:', err);
  } finally {
    await pool.end();
  }
}

seed();