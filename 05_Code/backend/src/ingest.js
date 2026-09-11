import { decodeTelemetry, TELEMETRY_FLAGS } from './telemetry.js';

function validSensorValue(decoded, flag, value) {
  return decoded.statusFlags & flag ? value : null;
}

export function sequenceQuality(previousSequence, sequenceNumber) {
  if (previousSequence === null || previousSequence === undefined) {
    return { sequenceState: 'first', missedCount: 0 };
  }
  if (sequenceNumber === previousSequence) {
    return { sequenceState: 'duplicate', missedCount: 0 };
  }
  if (sequenceNumber > previousSequence) {
    return {
      sequenceState: sequenceNumber - previousSequence === 1 ? 'in_order' : 'gap',
      missedCount: Math.max(0, sequenceNumber - previousSequence - 1),
    };
  }
  return { sequenceState: 'out_of_order', missedCount: 0 };
}

function extractLinkMetadata(body) {
  return {
    gateways: (body.uplink_message?.rx_metadata ?? []).map((metadata) => ({
      gatewayId: metadata.gateway_ids?.gateway_id ?? null,
      rssi: metadata.rssi ?? null,
      snr: metadata.snr ?? null,
      receivedAt: metadata.time ?? null,
    })),
    deviceId: body.end_device_ids?.device_id ?? null,
    applicationId: body.end_device_ids?.application_ids?.application_id ?? null,
  };
}

export function parseTtnUplink(body) {
  const encodedPayload = body?.uplink_message?.frm_payload;
  if (typeof encodedPayload !== 'string' || encodedPayload.length === 0) {
    throw new TypeError('TTN uplink no contiene uplink_message.frm_payload');
  }

  const payload = Buffer.from(encodedPayload, 'base64');
  const decoded = decodeTelemetry(payload);
  const receivedAt = body.received_at ?? body.uplink_message?.received_at ?? null;

  return {
    decoded,
    rawPayload: payload,
    nodeId: decoded.nodeId,
    sequenceNumber: decoded.sequenceNumber,
    receivedAt,
    ttnReceivedAt: receivedAt,
    linkMetadata: extractLinkMetadata(body),
  };
}

function qualityMetadata(decoded, sequence) {
  return {
    sequenceState: sequence.sequenceState,
    missedCount: sequence.missedCount,
    i2cBusError: Boolean(decoded.statusFlags & TELEMETRY_FLAGS.i2cBusError),
    sensors: decoded.sensorStatus,
  };
}

export async function ingestTtnUplink(pool, body) {
  const uplink = parseTtnUplink(body);
  const client = await pool.connect();

  try {
    await client.query('BEGIN');
    const previous = await client.query(
      'SELECT last_sequence_number FROM node_current_state WHERE node_id = $1 FOR UPDATE',
      [uplink.nodeId],
    );
    const previousSequence = previous.rows[0]?.last_sequence_number;
    const sequence = sequenceQuality(
      previousSequence === undefined ? null : Number(previousSequence),
      uplink.sequenceNumber,
    );
    const quality = qualityMetadata(uplink.decoded, sequence);

    await client.query(`
      INSERT INTO nodes (node_id, display_name)
      VALUES ($1, $2)
      ON CONFLICT (node_id) DO NOTHING
    `, [uplink.nodeId, `Nodo ${uplink.nodeId}`]);

    const inserted = await client.query(`
      INSERT INTO telemetry_measurements (
        node_id, sequence_number, received_at, ttn_received_at,
        temp_canopy_top_c, temp_canopy_mid_c, temp_canopy_low_c, temp_base_c,
        hum_base_pct, pressure_hpa, soil_moisture_pct, temp_soil_c,
        status_flags, raw_payload, link_metadata, quality_metadata
      ) VALUES (
        $1, $2, COALESCE($3::timestamptz, now()), $3::timestamptz,
        $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14::jsonb, $15::jsonb
      )
      ON CONFLICT (node_id, sequence_number) DO NOTHING
      RETURNING id
    `, [
      uplink.nodeId,
      uplink.sequenceNumber,
      uplink.receivedAt,
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.canopyTop, uplink.decoded.tempCanopyTopC),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.canopyMid, uplink.decoded.tempCanopyMidC),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.canopyLow, uplink.decoded.tempCanopyLowC),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.baseBme, uplink.decoded.tempBaseC),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.baseBme, uplink.decoded.humBasePct),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.baseBme, uplink.decoded.pressureHpa),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.soilMoisture, uplink.decoded.soilMoisturePct),
      validSensorValue(uplink.decoded, TELEMETRY_FLAGS.soilTemperature, uplink.decoded.tempSoilC),
      uplink.decoded.statusFlags,
      uplink.rawPayload,
      JSON.stringify(uplink.linkMetadata),
      JSON.stringify(quality),
    ]);

    if (inserted.rowCount === 0) {
      await client.query('COMMIT');
      return { status: 'duplicate', nodeId: uplink.nodeId, sequenceNumber: uplink.sequenceNumber };
    }

    const measurementId = inserted.rows[0].id;
    await client.query(`
      INSERT INTO node_current_state (
        node_id, measurement_id, last_sequence_number, last_received_at,
        connection_state, last_status_flags, quality_metadata
      ) VALUES ($1, $2, $3, COALESCE($4::timestamptz, now()), 'online', $5, $6::jsonb)
      ON CONFLICT (node_id) DO UPDATE SET
        measurement_id = EXCLUDED.measurement_id,
        last_sequence_number = EXCLUDED.last_sequence_number,
        last_received_at = EXCLUDED.last_received_at,
        connection_state = EXCLUDED.connection_state,
        last_status_flags = EXCLUDED.last_status_flags,
        quality_metadata = EXCLUDED.quality_metadata,
        updated_at = now()
      WHERE EXCLUDED.last_sequence_number >= node_current_state.last_sequence_number
    `, [
      uplink.nodeId,
      measurementId,
      uplink.sequenceNumber,
      uplink.receivedAt,
      uplink.decoded.statusFlags,
      JSON.stringify(quality),
    ]);
    await client.query('COMMIT');
    return { status: 'inserted', measurementId, nodeId: uplink.nodeId, sequenceNumber: uplink.sequenceNumber };
  } catch (error) {
    await client.query('ROLLBACK');
    throw error;
  } finally {
    client.release();
  }
}