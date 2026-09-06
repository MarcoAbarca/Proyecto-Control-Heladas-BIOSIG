/**
 * =============================================================================
 * sensor_manager.h
 * -----------------------------------------------------------------------------
 * ACTUALIZADO: reemplaza el set de sensores anterior (SHT31/MPU6050) por
 * el confirmado: 3x MLX90614 (estratos de copa, Canales 0-2 vía P82B715),
 * BME280 base (Canal 3, local), sonda capacitiva de suelo y batería
 * (ambas por ADC, sin pasar por el multiplexor I2C).
 * =============================================================================
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_Sensor.h>

#include "pca9548a.h"
#include "telemetry.h"

class SensorManager {
public:
    explicit SensorManager(PCA9548A &mux);

    /**
     * Lee los 3 MLX90614, el BME280 base, la sonda de suelo y la batería,
     * llenando `payload`. Cada sensor se intenta de forma independiente:
     * si uno falla, los demás igual se leen y se transmiten.
     *
     * @return true si AL MENOS una lectura fue exitosa.
     */
    bool readAllInto(TelemetryPayload &payload);

    /**
     * Lee SOLO la sonda de suelo y la batería (ambas por ADC, no pasan
     * por el mux I2C). Camino rápido para cuando mux.begin() falló: evita
     * gastar ~4 x I2C_TRANSACTION_TIMEOUT_MS en intentos de canal que ya
     * se sabe que van a fallar, con el CPU despierto y consumiendo.
     *
     * @return true si AL MENOS una de las dos lecturas fue exitosa.
     */
    bool readSoilAndBatteryOnly(TelemetryPayload &payload);

private:
    PCA9548A &_mux;

    Adafruit_MLX90614 _mlxTop;
    Adafruit_MLX90614 _mlxMid;
    Adafruit_MLX90614 _mlxLow;
    Adafruit_BME280 _bmeBase;

    bool _mlxTopReady = false;
    bool _mlxMidReady = false;
    bool _mlxLowReady = false;
    bool _bmeBaseReady = false;

    /**
     * Lee un MLX90614 en el canal indicado (aislando el canal a 100kHz
     * vía el mux). `mlx` y `ready` son la instancia de driver y el flag
     * de estado correspondientes a ESE canal específico. `outTempC_x100`
     * es una VARIABLE LOCAL del llamador (no un campo del struct packed):
     * tomar una referencia directa a un miembro de TelemetryPayload
     * (que es __attribute__((packed)), por lo tanto potencialmente
     * desalineado) no es seguro — el llamador debe copiar el valor al
     * struct después de que esta función retorne.
     */
    bool readMLX(uint8_t channel, Adafruit_MLX90614 &mlx, bool &ready,
                 int16_t &outTempC_x100, uint16_t okFlag, TelemetryPayload &payload);

    bool readBaseBME280(TelemetryPayload &payload);

    /** Sonda capacitiva de suelo, por ADC (no pasa por el mux I2C). */
    bool readSoilMoisture(TelemetryPayload &payload);

    /** Divisor resistivo de batería, por ADC. */
    void readBatteryVoltage(TelemetryPayload &payload);
};

#endif // SENSOR_MANAGER_H
