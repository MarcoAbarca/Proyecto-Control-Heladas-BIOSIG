import test from 'node:test';
import assert from 'node:assert/strict';
import { decodeTelemetry } from '../src/telemetry.js';

test('decodifica el vector de telemetria v1 en little-endian', () => {
  const payload = '2a78563412d20438ff9c00110ad711152755000dff0d03';
  const decoded = decodeTelemetry(payload);

  assert.deepEqual(decoded, {
    nodeId: 42,
    sequenceNumber: 0x12345678,
    tempCanopyTopC: 12.34,
    tempCanopyMidC: -2,
    tempCanopyLowC: 1.56,
    tempBaseC: 25.77,
    humBasePct: 45.67,
    pressureHpa: 1000.5,
    soilMoisturePct: 0.85,
    tempSoilC: -2.43,
    statusFlags: 0x030d,
    sensorStatus: {
      canopyTopOk: true,
      canopyMidOk: false,
      canopyLowOk: true,
      baseBmeOk: true,
      soilMoistureOk: false,
      soilTemperatureOk: false,
    },
    alerts: { frost: true, heat: true },
    rawPayloadHex: payload,
  });
});

test('rechaza payloads que no tienen 23 bytes', () => {
  assert.throws(() => decodeTelemetry('00'), /se esperaban 23 bytes/);
});

test('rechaza hexadecimal mal formado', () => {
  assert.throws(() => decodeTelemetry('not-hex'), /hexadecimal/);
});