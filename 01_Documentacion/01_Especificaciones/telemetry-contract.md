# Contrato de telemetria v1

El payload P2P/TTN contiene exactamente 23 bytes. Los enteros de mas de un
byte usan little-endian, igual que el `TelemetryPayload` packed compilado en
el ESP32.

| Offset | Bytes | Campo | Tipo | Escala |
| ---: | ---: | --- | --- | ---: |
| 0 | 1 | `nodeId` | uint8 | 1 |
| 1 | 4 | `sequenceNumber` | uint32 | 1 |
| 5 | 2 | `tempCanopyTop_x100` | int16 | 100 |
| 7 | 2 | `tempCanopyMid_x100` | int16 | 100 |
| 9 | 2 | `tempCanopyLow_x100` | int16 | 100 |
| 11 | 2 | `tempBaseC_x100` | int16 | 100 |
| 13 | 2 | `humBasePct_x100` | uint16 | 100 |
| 15 | 2 | `pressureHpa_x10` | uint16 | 10 |
| 17 | 2 | `soilMoisturePct_x100` | uint16 | 100 |
| 19 | 2 | `tempSoilC_x100` | int16 | 100 |
| 21 | 2 | `statusFlags` | uint16 | 1 |

## Flags

- `0x0001`: MLX90614 superior correcto
- `0x0002`: MLX90614 medio correcto
- `0x0004`: MLX90614 inferior correcto
- `0x0008`: BME280 correcto
- `0x0010`: humedad de suelo correcta
- `0x0020`: DS18B20 correcto
- `0x0040`: error del bus I2C
- `0x0100`: alerta de helada
- `0x0200`: alerta de temperatura alta

El payload no contiene timestamp de medicion. El backend debe asignar el
timestamp de recepcion y conservar el timestamp de TTN cuando este disponible.
Los valores escalados solo se consideran validos cuando el flag del sensor
correspondiente esta activo.