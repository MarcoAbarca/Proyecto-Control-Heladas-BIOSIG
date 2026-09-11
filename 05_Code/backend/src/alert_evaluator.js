/**
 * Evaluador simplificado de alertas para Sensagri (Versión MVP)
 */
export async function evaluateTelemetryAlerts(pool, measurement) {
  try {
    const nodeId = measurement.nodeId ?? measurement.node_id;
    if (nodeId === undefined || nodeId === null) return;

    // Obtener la configuración del nodo
    const settingsRes = await pool.query(`
      SELECT frost_enabled AS "frostEnabled", frost_threshold_c AS "frostThresholdC"
      FROM node_alert_settings WHERE node_id = $1
    `, [nodeId]);

    const settings = settingsRes.rows[0] ?? {
      frostEnabled: true,
      frostThresholdC: 0.0
    };

    // Obtener la menor temperatura registrada entre los sensores disponibles
    const top = measurement.tempCanopyTopC ?? measurement.temp_canopy_top_c;
    const mid = measurement.tempCanopyMidC ?? measurement.temp_canopy_mid_c;
    const low = measurement.tempCanopyLowC ?? measurement.temp_canopy_low_c;
    const base = measurement.tempBaseC ?? measurement.temp_base_c;

    const validTemps = [top, mid, low, base]
      .filter((val) => val !== null && val !== undefined && Number.isFinite(Number(val)))
      .map(Number);

    if (validTemps.length === 0) return;

    const minTemp = Math.min(...validTemps);

    // Evaluar si cae bajo el umbral (ej. <= 0.0 °C)
    if (settings.frostEnabled && minTemp <= settings.frostThresholdC) {
      const activeAlert = await pool.query(`
        SELECT id FROM alert_events 
        WHERE node_id = $1 AND alert_type = 'frost' AND state = 'active'
      `, [nodeId]);

      if (activeAlert.rows.length === 0) {
        await pool.query(`
          INSERT INTO alert_events (node_id, alert_type, severity, state, message, value, started_at)
          VALUES ($1, 'frost', 'critical', 'active', $2, $3, NOW())
        `, [
          nodeId,
          `⚠️ Helada: ${minTemp.toFixed(2)} °C en Nodo #${nodeId}`,
          minTemp
        ]);
        console.log(`[ALERT] 🚨 Helada registrada en Nodo #${nodeId}: ${minTemp} °C`);
      }
    } else {
      // Auto-resolver alerta si la temperatura se normaliza
      await pool.query(`
        UPDATE alert_events 
        SET state = 'resolved', resolved_at = NOW()
        WHERE node_id = $1 AND alert_type = 'frost' AND state = 'active'
      `, [nodeId]);
    }
  } catch (err) {
    console.error('Error evaluando alertas:', err.message);
  }
}