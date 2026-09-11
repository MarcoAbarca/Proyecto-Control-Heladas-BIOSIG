import test from 'node:test';
import assert from 'node:assert/strict';
import { parseTtnUplink, sequenceQuality } from '../src/ingest.js';

const payloadHex = '2a78563412d20438ff9c00110ad711152755000dff0d03';

test('extrae y decodifica un uplink JSON de TTN', () => {
  const result = parseTtnUplink({
    received_at: '2026-09-07T12:00:00.000Z',
    end_device_ids: {
      device_id: 'nodo-42',
      application_ids: { application_id: 'heladas' },
    },
    uplink_message: {
      frm_payload: Buffer.from(payloadHex, 'hex').toString('base64'),
      rx_metadata: [{ gateway_ids: { gateway_id: 'gateway-curico' }, rssi: -91, snr: 7.5 }],
    },
  });

  assert.equal(result.nodeId, 42);
  assert.equal(result.sequenceNumber, 0x12345678);
  assert.equal(result.decoded.tempBaseC, 25.77);
  assert.equal(result.linkMetadata.gateways[0].gatewayId, 'gateway-curico');
  assert.equal(result.rawPayload.toString('hex'), payloadHex);
});

test('rechaza uplinks TTN sin payload binario', () => {
  assert.throws(() => parseTtnUplink({ uplink_message: {} }), /frm_payload/);
});

test('clasifica correctamente secuencia inicial, salto y fuera de orden', () => {
  assert.deepEqual(sequenceQuality(null, 0), { sequenceState: 'first', missedCount: 0 });
  assert.deepEqual(sequenceQuality(4, 7), { sequenceState: 'gap', missedCount: 2 });
  assert.deepEqual(sequenceQuality(7, 6), { sequenceState: 'out_of_order', missedCount: 0 });
});