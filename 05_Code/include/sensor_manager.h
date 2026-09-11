/**
 * =============================================================================
 * sensor_manager.h
 * -----------------------------------------------------------------------------
 * ACTUALIZADO: reemplaza el set de sensores anterior (SHT31/MPU6050) por
 * el confirmado: 3x MLX90614 (estratos de copa, Canales 0-2 vía P82B715),
 * BME280 base (Canal 3, local), sonda capacitiva de suelo y DS18B20
 * (ADC/1-Wire, sin pasar por el multiplexor I2C).
 * =============================================================================
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_Sensor.h>

#include "pca9548a.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "telemetry.h"

class SensorManager {
public:
    explicit SensorManager(PCA9548A &mux);

    /**
    * Lee los 3 MLX90614, el BME280 base y la sonda de suelo,
     * llenando `payload`. Cada sensor se intenta de forma independiente:
     * si uno falla, los demás igual se leen y se transmiten.
     *
    * @return true si AL MENOS un sensor I2C (MLX90614 o BME280) respondió.
     */
    bool readAllInto(TelemetryPayload &payload);

    /**
    * Lee SOLO la sonda de suelo por ADC (no pasa
     * por el mux I2C). Camino rápido para cuando mux.begin() falló: evita
     * gastar ~4 x I2C_TRANSACTION_TIMEOUT_MS en intentos de canal que ya
     * se sabe que van a fallar, con el CPU despierto y consumiendo.
     *
    * @return false siempre: este camino no realiza lecturas I2C y su
    *         resultado no debe reiniciar el diagnóstico del bus.
     */
    bool readSoilOnly(TelemetryPayload &payload);

private:
    PCA9548A &_mux;

    Adafruit_MLX90614 _mlxTop;
    Adafruit_MLX90614 _mlxMid;
    Adafruit_MLX90614 _mlxLow;
    Adafruit_BME280 _bmeBase;
    OneWire _soilOneWire;
    DallasTemperature _soilTemperature;

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

        /** DS18B20 de suelo, por 1-Wire en D7. */
        bool readSoilTemperature(TelemetryPayload &payload);

    bool readBaseBME280(TelemetryPayload &payload);

    /** Sonda capacitiva de suelo, por ADC (no pasa por el mux I2C). */
    bool readSoilMoisture(TelemetryPayload &payload);

};

#endif // SENSOR_MANAGER_H
