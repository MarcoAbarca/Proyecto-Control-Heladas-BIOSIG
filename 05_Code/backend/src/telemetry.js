export const TELEMETRY_PAYLOAD_SIZE = 23;

const SENSOR_FLAGS = Object.freeze({
  canopyTop: 0x0001,
  canopyMid: 0x0002,
  canopyLow: 0x0004,
  baseBme: 0x0008,
  soilMoisture: 0x0010,
  soilTemperature: 0x0020,
});

export const TELEMETRY_FLAGS = Object.freeze({
  ...SENSOR_FLAGS,
  i2cBusError: 0x0040,
  frostAlert: 0x0100,
  heatAlert: 0x0200,
});

function readPayloadBuffer(payload) {
  if (payload instanceof Uint8Array) {
    return Buffer.from(payload);
  }

  if (typeof payload !== 'string' || !/^[0-9a-fA-F]*$/.test(payload) || payload.length % 2 !== 0) {
    throw new TypeError('El payload debe ser hexadecimal con longitud par');
  }

  return Buffer.from(payload, 'hex');
}

function scaled(value, factor) {
  return value / factor;
}

export function decodeTelemetry(payload) {
  const buffer = readPayloadBuffer(payload);
  if (buffer.length !== TELEMETRY_PAYLOAD_SIZE) {
    throw new RangeError(`Payload invalido: se esperaban ${TELEMETRY_PAYLOAD_SIZE} bytes y llegaron ${buffer.length}`);
  }

  const statusFlags = buffer.readUInt16LE(21);
  return {
    nodeId: buffer.readUInt8(0),
    sequenceNumber: buffer.readUInt32LE(1),
    tempCanopyTopC: scaled(buffer.readInt16LE(5), 100),
    tempCanopyMidC: scaled(buffer.readInt16LE(7), 100),
    tempCanopyLowC: scaled(buffer.readInt16LE(9), 100),
    tempBaseC: scaled(buffer.readInt16LE(11), 100),
    humBasePct: scaled(buffer.readUInt16LE(13), 100),
    pressureHpa: scaled(buffer.readUInt16LE(15), 10),
    soilMoisturePct: scaled(buffer.readUInt16LE(17), 100),
    tempSoilC: scaled(buffer.readInt16LE(19), 100),
    statusFlags,
    sensorStatus: {
      canopyTopOk: Boolean(statusFlags & SENSOR_FLAGS.canopyTop),
      canopyMidOk: Boolean(statusFlags & SENSOR_FLAGS.canopyMid),
      canopyLowOk: Boolean(statusFlags & SENSOR_FLAGS.canopyLow),
      baseBmeOk: Boolean(statusFlags & SENSOR_FLAGS.baseBme),
      soilMoistureOk: Boolean(statusFlags & SENSOR_FLAGS.soilMoisture),
      soilTemperatureOk: Boolean(statusFlags & SENSOR_FLAGS.soilTemperature),
    },
    alerts: {
      frost: Boolean(statusFlags & TELEMETRY_FLAGS.frostAlert),
      heat: Boolean(statusFlags & TELEMETRY_FLAGS.heatAlert),
    },
    rawPayloadHex: buffer.toString('hex'),
  };
}