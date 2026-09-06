/**
 * =============================================================================
 * telemetry.h
 * -----------------------------------------------------------------------------
 * ACTUALIZADO: reemplaza el struct anterior (SHT3x externo + BME280 local +
 * MPU6050) por el que corresponde a la arquitectura confirmada: 3x
 * MLX90614 (estratos de copa) + BME280 base + humedad de suelo + batería.
 *
 * Este archivo NO estaba en la lista de 4 que pediste actualizar, pero es
 * imprescindible: sensor_manager.cpp no puede llenar campos que ya no
 * existen (tempExtC, accelX_mg, etc. no tienen sentido con los sensores
 * reales). Lo reescribo ahora para que todo compile de forma consistente;
 * avisame si preferías revisar el layout de campos antes de aplicarlo.
 *
 * __attribute__((packed)) sigue siendo obligatorio por la misma razón que
 * antes: sin él, el compilador puede insertar padding entre campos.
 * =============================================================================
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

// =============================================================================
// Factores de escala
// =============================================================================
constexpr float TEMP_SCALE_FACTOR      = 100.0f;  // °C  -> x100
constexpr float HUMIDITY_SCALE_FACTOR  = 100.0f;  // %RH -> x100
constexpr float PRESSURE_SCALE_FACTOR  = 10.0f;   // hPa -> x10
constexpr float SOIL_MOISTURE_SCALE_FACTOR = 100.0f; // % -> x100

// =============================================================================
// Bits de TelemetryPayload::statusFlags (16 bits: byte bajo = salud de
// sensores, byte alto = alertas agronómicas de negocio)
// -----------------------------------------------------------------------------
// ACTUALIZADO: ensanchado de 8 a 16 bits. Los 8 bits originales ya estaban
// completos (5 flags de sensor + batería + 2 alertas) y no quedaba lugar
// para FLAG_I2C_BUS_ERROR (necesaria para el camino rápido de la Sección
// 1 del brief). De paso, esto alinea los valores de FLAG_ALERT_FROST/
// FLAG_ALERT_HEAT_WINTER exactamente con 0x0100/0x0200 tal como los pidió
// Gemini para el decoder de TTN.
// =============================================================================
constexpr uint16_t FLAG_MLX_TOP_OK     = 1 << 0; // Canal 0: MLX90614 estrato superior
constexpr uint16_t FLAG_MLX_MID_OK     = 1 << 1; // Canal 1: MLX90614 estrato medio
constexpr uint16_t FLAG_MLX_LOW_OK     = 1 << 2; // Canal 2: MLX90614 estrato inferior
constexpr uint16_t FLAG_BASE_BME_OK    = 1 << 3; // Canal 3: BME280 base
constexpr uint16_t FLAG_SOIL_OK        = 1 << 4; // Sonda capacitiva de suelo (ADC)
constexpr uint16_t FLAG_LOW_BATTERY    = 1 << 5; // Voltaje de batería bajo
constexpr uint16_t FLAG_I2C_BUS_ERROR  = 1 << 6; // mux.begin() fallo este ciclo (bus caido/no recuperado)
// Bit 7 libre en el byte bajo (salud de sensores).

constexpr uint16_t FLAG_ALERT_FROST      = 0x0100; // Temp <= FROST_THRESHOLD_C
constexpr uint16_t FLAG_ALERT_HEAT_WINTER = 0x0200; // Temp >= OVERHEAT_THRESHOLD_C
// Bits 10-15 libres en el byte alto (alertas de negocio futuras).

// =============================================================================
// TelemetryPayload - estructura transmitida cruda por el radio
// =============================================================================
#pragma pack(push, 1)
struct TelemetryPayload {
    uint8_t  nodeId;
    uint32_t sequenceNumber;      // msg_counter del ICD; sobrevive en RTC_DATA_ATTR

    int16_t  tempCanopyTop_x100;  // Canal 0: MLX90614 estrato superior
    int16_t  tempCanopyMid_x100;  // Canal 1: MLX90614 estrato medio
    int16_t  tempCanopyLow_x100;  // Canal 2: MLX90614 estrato inferior

    int16_t  tempBaseC_x100;      // Canal 3: BME280 base - temperatura
    uint16_t humBasePct_x100;     // Canal 3: BME280 base - humedad
    uint16_t pressureHpa_x10;     // Canal 3: BME280 base - presión

    uint16_t soilMoisturePct_x100; // Sonda capacitiva de suelo (ADC)

    uint16_t batteryMilliVolts;   // Voltaje de batería en mV
    uint16_t statusFlags;         // ACTUALIZADO: 8 -> 16 bits, ver flags arriba
} __attribute__((packed));
#pragma pack(pop)

// 1+4+2+2+2+2+2+2+2+2+2 = 23 bytes (antes 22: statusFlags paso de 1 a 2
// bytes). Este es el layout real que debe reflejar el payload formatter
// de TTN -- ver la corrección de decodeUplink() enviada en esta misma
// entrega, el que tenían asumía un layout distinto (sin nodeId, y con
// statusFlags de 2 bytes pero en otra posición).
static_assert(sizeof(TelemetryPayload) == 23,
    "TelemetryPayload cambio de tamano: verificar Time-on-Air y actualizar "
    "el ICD/documentacion y el decodeUplink() del Payload Formatter en TTN.");

namespace Telemetry {

    inline int16_t packScaledInt16(float value, float scale) {
        return static_cast<int16_t>(value * scale);
    }

    inline uint16_t packScaledUInt16(float value, float scale) {
        return static_cast<uint16_t>(value * scale);
    }

    inline float unpackScaled(int16_t raw, float scale) {
        return static_cast<float>(raw) / scale;
    }

    inline float unpackScaled(uint16_t raw, float scale) {
        return static_cast<float>(raw) / scale;
    }

    inline TelemetryPayload makeEmptyPayload(uint8_t nodeId, uint32_t sequenceNumber) {
        TelemetryPayload p{};
        p.nodeId = nodeId;
        p.sequenceNumber = sequenceNumber;
        return p;
    }

} // namespace Telemetry

#endif // TELEMETRY_H
