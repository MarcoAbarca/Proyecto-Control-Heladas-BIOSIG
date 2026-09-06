/**
 * =============================================================================
 * payload_formatter.js -- Decoder de Uplink para TTN (Console > Payload Formatters)
 * -----------------------------------------------------------------------------
 * CORREGIDO: la versión anterior no coincidía con el layout real de
 * TelemetryPayload (telemetry.h). Diferencias encontradas:
 *   1. Faltaba `nodeId` (byte 0) -- no se leía en absoluto.
 *   2. `sequenceNumber` estaba ubicado al final (bytes 16-19) cuando en el
 *      struct real va justo después de nodeId (bytes 1-4).
 *   3. `statusFlags` se leía como si estuviera en bytes[20-21], pero con
 *      nodeId+sequenceNumber corridos, la posición real es bytes[21-22].
 *   4. El struct C++ tiene 23 bytes totales (no 22): statusFlags se
 *      ensanchó de 8 a 16 bits para poder alojar FLAG_ALERT_FROST/
 *      FLAG_ALERT_HEAT_WINTER en 0x0100/0x0200 como se pidió.
 *
 * El ESP32-S3 (Xtensa) es little-endian, así que el criterio LE del
 * decoder original era correcto -- lo que fallaba era el MAPEO de bytes a
 * campos, no el orden de bits dentro de cada campo.
 *
 * TABLA DE OFFSETS (debe mantenerse sincronizada con telemetry.h):
 *   byte  0      : nodeId            (uint8)
 *   bytes 1-4    : sequenceNumber    (uint32 LE)
 *   bytes 5-6    : tempCanopyTop_x100 (int16 LE)
 *   bytes 7-8    : tempCanopyMid_x100 (int16 LE)
 *   bytes 9-10   : tempCanopyLow_x100 (int16 LE)
 *   bytes 11-12  : tempBaseC_x100     (int16 LE)
 *   bytes 13-14  : humBasePct_x100    (uint16 LE)
 *   bytes 15-16  : pressureHpa_x10    (uint16 LE)
 *   bytes 17-18  : soilMoisturePct_x100 (uint16 LE)
 *   bytes 19-20  : batteryMilliVolts  (uint16 LE)
 *   bytes 21-22  : statusFlags        (uint16 LE)
 *   TOTAL: 23 bytes
 * =============================================================================
 */

function decodeUplink(input) {
    var bytes = input.bytes;
    if (bytes.length < 23) {
        return { errors: ["Payload corto (< 23 bytes): " + bytes.length] };
    }

    function readUInt8(offset) {
        return bytes[offset];
    }

    function readInt16LE(offsetLow) {
        var val = (bytes[offsetLow + 1] << 8) | bytes[offsetLow];
        return (val & 0x8000) ? val - 0x10000 : val;
    }

    function readUInt16LE(offsetLow) {
        return (bytes[offsetLow + 1] << 8) | bytes[offsetLow];
    }

    function readUInt32LE(offsetLow) {
        return (
            (bytes[offsetLow + 3] << 24) |
            (bytes[offsetLow + 2] << 16) |
            (bytes[offsetLow + 1] << 8) |
            bytes[offsetLow]
        ) >>> 0; // >>> 0 fuerza unsigned de 32 bits (evita signo negativo en JS)
    }

    var statusFlags = readUInt16LE(21);

    return {
        data: {
            nodeId:         readUInt8(0),
            sequenceNumber: readUInt32LE(1),

            tempCanopyTopC: readInt16LE(5) / 100.0,
            tempCanopyMidC: readInt16LE(7) / 100.0,
            tempCanopyLowC: readInt16LE(9) / 100.0,

            tempBaseC:     readInt16LE(11) / 100.0,
            humidityBase:  readUInt16LE(13) / 100.0,
            pressureBase:  readUInt16LE(15) / 10.0, // OJO: escala x10, no x100

            soilMoisturePct: readUInt16LE(17) / 100.0,
            batteryMilliVolts: readUInt16LE(19),

            statusFlags: statusFlags,
            flags: {
                mlxTopOk:      (statusFlags & 0x0001) !== 0,
                mlxMidOk:      (statusFlags & 0x0002) !== 0,
                mlxLowOk:      (statusFlags & 0x0004) !== 0,
                baseBmeOk:     (statusFlags & 0x0008) !== 0,
                soilOk:        (statusFlags & 0x0010) !== 0,
                lowBattery:    (statusFlags & 0x0020) !== 0,
                i2cBusError:   (statusFlags & 0x0040) !== 0,
                frostRisk:      (statusFlags & 0x0100) !== 0,
                winterHeatRisk: (statusFlags & 0x0200) !== 0
            }
        }
    };
}
