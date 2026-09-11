import { createPool } from './db.js';

const pool = createPool();

async function seedSecondNode() {
  console.log('🌱 Insertando datos de prueba para Nodo #2 (Sector Bajo Rengo)...');

  try {
    // 1. Registrar / Actualizar el Nodo #2 en la tabla de nodos
    await pool.query(`
      INSERT INTO nodes (node_id, display_name, enabled, communication_timeout_seconds)
      VALUES (2, 'Nodo 2 - Sector Bajo Rengo', true, 1800)
      ON CONFLICT (node_id) DO UPDATE 
      SET display_name = EXCLUDED.display_name;
    `);

    // 2. Configuración inicial de alertas para el Nodo #2
    await pool.query(`
      INSERT INTO node_alert_settings (
        node_id, frost_enabled, frost_threshold_c, heat_enabled, heat_threshold_c, telegram_enabled
      )
      VALUES (2, true, 1.5, true, 28.0, true)
      ON CONFLICT (node_id) DO NOTHING;
    `);

    // 3. Generar historial de 24 mediciones (cada 15 min) simulando descenso térmico bajo 0 °C
    const now = new Date();
    let lastInsertedId = null;
    let lastSeq = 200;
    const defaultFlags = 0x003F; // Flags de sensores OK
    const mockRawPayload = Buffer.from('0200000000000000000000000000000000000000000000', 'hex');

    for (let i = 24; i >= 0; i--) {
      const timestamp = new Date(now.getTime() - i * 15 * 60 * 1000);
      const seq = 200 - i; // Secuencias 176 a 200

      // Simular descenso de temperatura progresivo
      const tempBase = parseFloat((2.5 - (24 - i) * 0.25).toFixed(2));
      const tempCanopyTop = parseFloat((tempBase + 0.50).toFixed(2));
      const tempCanopyMid = parseFloat((tempBase + 0.20).toFixed(2));
      const tempCanopyLow = parseFloat((tempBase - 0.40).toFixed(2));

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
        2, seq, timestamp,
        tempCanopyTop, tempCanopyMid, tempCanopyLow,
        tempBase, 85.50, 1012.80,
        42.00, 9.10, defaultFlags, mockRawPayload
      ]);

      if (i === 0 && res.rows.length > 0) {
        lastInsertedId = res.rows[0].id;
        lastSeq = seq;
      }
    }

    if (!lastInsertedId) {
      const lastRow = await pool.query(`
        SELECT id FROM telemetry_measurements 
        WHERE node_id = 2 ORDER BY received_at DESC LIMIT 1;
      `);
      if (lastRow.rows.length > 0) {
        lastInsertedId = lastRow.rows[0].id;
      }
    }

    // 4. Actualizar el estado actual del Nodo #2
    if (lastInsertedId) {
      await pool.query(`
        INSERT INTO node_current_state (
          node_id, measurement_id, last_sequence_number, last_received_at, 
          connection_state, last_status_flags, quality_metadata, updated_at
        ) VALUES (
          2, $1, $2, $3, 'online', $4, '{}'::jsonb, $3
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

    console.log('✅ ¡Nodo #2 (Sector Bajo Rengo) insertado con éxito! Ahora tienes 2 nodos en la base de datos.');
  } catch (err) {
    console.error('❌ Error al insertar Nodo #2:', err.message);
  } finally {
    await pool.end();
  }
}

seedSecondNode();