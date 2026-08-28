/**
 * =============================================================================
 * telemetry.h
 * -----------------------------------------------------------------------------
 * Estructura binaria compacta transmitida por LoRa P2P desde el Nodo
 * Emisor hacia el Nodo Receptor. Usada en AMBOS roles:
 *   - Emisor: la llena a partir de las lecturas de sensores y la envía.
 *   - Receptor: la recibe cruda desde el radio, la interpreta (misma
 *     definición de struct en ambos binarios) y la vuelca a CSV/SQL.
 *
 * PRINCIPIO DE DISEÑO: todo valor de punto flotante se convierte a un
 * entero escalado antes de transmitir. Un float ocupa 4 bytes; un int16_t
 * escalado ocupa 2 y alcanza la precisión necesaria para variables
 * ambientales (ej. 0.01°C de resolución con temp_x100). Esto reduce el
 * payload total y, por lo tanto, el Time-on-Air de cada transmisión.
 *
 * `__attribute__((packed))` es obligatorio: sin él, el compilador podría
 * insertar bytes de relleno (padding) entre campos para alinear a 4 bytes,
 * lo que haría que el tamaño real transmitido no coincida con sizeof()
 * calculado a mano ni con lo que espera el Receptor.
 * =============================================================================
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

// =============================================================================
// Factores de escala. Se documentan aquí (no solo en el código de empaque)
// porque el Receptor los necesita exactamente iguales para desempaquetar.
// =============================================================================
constexpr float TEMP_SCALE_FACTOR      = 100.0f;  // °C  -> x100  (resolución 0.01°C)
constexpr float HUMIDITY_SCALE_FACTOR  = 100.0f;  // %RH -> x100  (resolución 0.01%)
constexpr float PRESSURE_SCALE_FACTOR  = 10.0f;   // hPa -> x10   (resolución 0.1 hPa)
constexpr float ACCEL_SCALE_FACTOR     = 1000.0f; // g   -> x1000 (resolución 0.001g, en mg)
constexpr float GYRO_SCALE_FACTOR      = 10.0f;   // °/s -> x10   (resolución 0.1 °/s)

// =============================================================================
// Bits de TelemetryPayload::statusFlags
// -----------------------------------------------------------------------------
// Permite que el Receptor sepa, sin ambigüedad, qué campos del payload son
// datos válidos y cuáles quedaron en su valor por defecto porque el sensor
// correspondiente falló o aún no tiene driver (ver SensorStatus en
// sensor_manager.h). Evita que un 0x0000 "silencioso" se confunda con una
// lectura real de 0.
// =============================================================================
constexpr uint8_t FLAG_EXT_SENSOR_OK   = 1 << 0; // Canal 0 (SHT3x/BME280 externo)
constexpr uint8_t FLAG_LOCAL_BME_OK    = 1 << 1; // Canal 1 (BME280 local)
constexpr uint8_t FLAG_MPU6050_OK      = 1 << 2; // Canal 2 (MPU6050)
constexpr uint8_t FLAG_LOW_BATTERY     = 1 << 3; // Bajo voltaje de batería
constexpr uint8_t FLAG_BUS_RECOVERY_USED = 1 << 4; // Hubo que recuperar el bus I2C este ciclo

// =============================================================================
// TelemetryPayload - estructura transmitida cruda por el radio
// =============================================================================
#pragma pack(push, 1)
struct TelemetryPayload {
    uint8_t  nodeId;              // Identificador del nodo emisor (config.h: NODE_ID)
    uint32_t sequenceNumber;      // Contador incremental de ciclos (no hay RTC en el nodo)

    // --- Canal 0: sensor externo (SHT3x/BME280 vía P82B715/Cat6) ----------
    int16_t  tempExtC_x100;       // Temperatura externa x100
    uint16_t humExtPct_x100;      // Humedad externa x100

    // --- Canal 1: BME280 local (base/suelo) --------------------------------
    int16_t  tempLocalC_x100;     // Temperatura local x100
    uint16_t humLocalPct_x100;    // Humedad local x100
    uint16_t pressureHpa_x10;     // Presión atmosférica x10

    // --- Canal 2: MPU6050 (inclinación/estructura) -------------------------
    int16_t  accelX_mg;           // Aceleración eje X, en mili-g
    int16_t  accelY_mg;           // Aceleración eje Y, en mili-g
    int16_t  accelZ_mg;           // Aceleración eje Z, en mili-g
    int16_t  gyroX_dps_x10;       // Velocidad angular eje X x10 (°/s)
    int16_t  gyroY_dps_x10;       // Velocidad angular eje Y x10 (°/s)
    int16_t  gyroZ_dps_x10;       // Velocidad angular eje Z x10 (°/s)

    // --- Estado del nodo -----------------------------------------------------
    uint16_t batteryMilliVolts;   // Voltaje de batería en mV (lectura directa, sin escalar)
    uint8_t  statusFlags;         // Ver bits FLAG_* arriba
} __attribute__((packed));
#pragma pack(pop)

// Verificación en tiempo de compilación: si alguien agrega un campo y el
// tamaño no es el esperado, el build falla aquí en vez de descubrirse
// como un bug de payload corrupto en el Receptor. Actualizar el número si
// se modifica la estructura intencionalmente.
static_assert(sizeof(TelemetryPayload) == 30,
    "TelemetryPayload cambio de tamano: verificar impacto en Time-on-Air "
    "y actualizar el desempaquetado en el Receptor.");

// =============================================================================
// Utilidades de empaquetado / desempaquetado
// -----------------------------------------------------------------------------
// Funciones puras (sin efectos secundarios) para convertir entre el valor
// físico real (float) y su representación escalada en el payload. Se usan
// tanto en el Emisor (float -> escalado, al llenar el struct) como en el
// Receptor (escalado -> float, al leer el struct recibido para CSV/SQL).
// =============================================================================
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

    /**
     * Devuelve un TelemetryPayload en su estado "vacío" seguro: todos los
     * campos numéricos en 0 y statusFlags en 0 (ningún sensor marcado como
     * OK todavía). Se usa como punto de partida al inicio de cada ciclo de
     * lectura del Emisor, antes de ir llenando campo por campo según cada
     * sensor responda.
     */
    inline TelemetryPayload makeEmptyPayload(uint8_t nodeId, uint32_t sequenceNumber) {
        TelemetryPayload p{};
        p.nodeId = nodeId;
        p.sequenceNumber = sequenceNumber;
        return p;
    }

} // namespace Telemetry

#endif // TELEMETRY_H
