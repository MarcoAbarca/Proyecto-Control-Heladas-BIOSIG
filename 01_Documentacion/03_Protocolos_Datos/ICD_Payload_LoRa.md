# ICD: Payload LoRa P2P
> Fuente de verdad: `05_Code/include/telemetry.h`. Ante discrepancia, el código manda.

## 1. Capa Física
* LoRa P2P (no LoRaWAN) Emisor→Receptor. Solo el Receptor habla LoRaWAN hacia TTN.
* 915.0 MHz, BW 125kHz, **SF7 fijo**, CR 4/5, Sync Word `0x12` (privada), Preamble 8, TX +22dBm.
* **Tamaño fijo: 23 bytes** (antes documentado como 15 — desactualizado).

## 2. Mapa de Bytes (`TelemetryPayload`, packed)
| Offset | Campo | Tipo | Escala | Descripción |
|---|---|---|---|---|
|0|nodeId|uint8_t|1|ID del nodo|
|1-4|sequenceNumber|uint32_t|1|Contador, persiste en RTC_DATA_ATTR|
|5-6|tempCanopyTop_x100|int16_t|x100|MLX90614 Canal 0|
|7-8|tempCanopyMid_x100|int16_t|x100|MLX90614 Canal 1|
|9-10|tempCanopyLow_x100|int16_t|x100|MLX90614 Canal 2|
|11-12|tempBaseC_x100|int16_t|x100|BME280 Canal 3, temp|
|13-14|humBasePct_x100|uint16_t|x100|BME280 Canal 3, hum|
|15-16|pressureHpa_x10|uint16_t|**x10**|BME280 Canal 3, presión|
|17-18|soilMoisturePct_x100|uint16_t|x100|ADC D6, no pasa por mux|
|19-20|batteryMilliVolts|uint16_t|1|ADC D7 x BATTERY_DIVIDER_RATIO|
|21-22|statusFlags|uint16_t|bitfield|Ver tabla abajo|

⚠️ No existe campo de temperatura de suelo (NTC/DS18B20): el BOM lo lista pero el firmware actual no lo implementa, solo humedad capacitiva.

## 3. Bits de statusFlags
|Bit|Constante|Significado|
|---|---|---|
|0|FLAG_MLX_TOP_OK|Canal 0 leyó OK|
|1|FLAG_MLX_MID_OK|Canal 1 leyó OK|
|2|FLAG_MLX_LOW_OK|Canal 2 leyó OK|
|3|FLAG_BASE_BME_OK|Canal 3 leyó OK|
|4|FLAG_SOIL_OK|ADC suelo leído|
|5|FLAG_LOW_BATTERY|batería < 3400mV|
|6|FLAG_I2C_BUS_ERROR|mux.begin() falló este ciclo|
|8 (0x0100)|FLAG_ALERT_FROST|Temp ref ≤ 2.0°C|
|9 (0x0200)|FLAG_ALERT_HEAT_WINTER|Temp ref > 15.0°C|

Temp de referencia para alertas: MLX top si OK, sino BME280 base (`evaluateAlerts()` en main.cpp).

## 4. Struct real (referencia, ver telemetry.h)
```cpp
#pragma pack(push, 1)
struct TelemetryPayload {
    uint8_t nodeId; uint32_t sequenceNumber;
    int16_t tempCanopyTop_x100, tempCanopyMid_x100, tempCanopyLow_x100, tempBaseC_x100;
    uint16_t humBasePct_x100, pressureHpa_x10, soilMoisturePct_x100, batteryMilliVolts, statusFlags;
} __attribute__((packed));
#pragma pack(pop)
static_assert(sizeof(TelemetryPayload) == 23, "...");
```

## 5. Decodificación
* Receptor: `checkReceivedP2P()` en lora_manager.cpp hace memcpy directo sobre TelemetryPayload — Emisor y Receptor deben compilar el mismo telemetry.h.
* TTN: `04_Panel_Nube/payload_formatter.js` decodifica los 23 bytes en JS (tabla de offsets documentada dentro del propio archivo).
* ESP32-S3 es little-endian.
