/**
 * =============================================================================
 * sensor_manager.cpp
 * =============================================================================
 */

#include "sensor_manager.h"
#include "config.h"
#include "i2c_bus.h"

SensorManager::SensorManager(PCA9548A &mux) : _mux(mux) {}

// -----------------------------------------------------------------------------
// readAllInto()
// -----------------------------------------------------------------------------
bool SensorManager::readAllInto(TelemetryPayload &payload) {
    bool anyOk = false;

    // Variables LOCALES (no referencias a campos packed) — ver nota en
    // sensor_manager.h sobre por qué readMLX() no escribe directo al struct.
    int16_t tempTop = 0, tempMid = 0, tempLow = 0;

    if (readMLX(CH_MLX_TOP, _mlxTop, _mlxTopReady, tempTop, FLAG_MLX_TOP_OK, payload)) {
        payload.tempCanopyTop_x100 = tempTop;
        anyOk = true;
    }
    if (readMLX(CH_MLX_MID, _mlxMid, _mlxMidReady, tempMid, FLAG_MLX_MID_OK, payload)) {
        payload.tempCanopyMid_x100 = tempMid;
        anyOk = true;
    }
    if (readMLX(CH_MLX_LOW, _mlxLow, _mlxLowReady, tempLow, FLAG_MLX_LOW_OK, payload)) {
        payload.tempCanopyLow_x100 = tempLow;
        anyOk = true;
    }

    anyOk |= readBaseBME280(payload);

    _mux.disableAll(); // Estado seguro: cierra canal I2C antes de pasar al ADC.

    anyOk |= readSoilMoisture(payload);
    readBatteryVoltage(payload); // Siempre se intenta; no participa del "anyOk".

    return anyOk;
}

// -----------------------------------------------------------------------------
// readSoilAndBatteryOnly() - camino rápido cuando el mux falló
// -----------------------------------------------------------------------------
bool SensorManager::readSoilAndBatteryOnly(TelemetryPayload &payload) {
    bool anyOk = readSoilMoisture(payload);
    readBatteryVoltage(payload);
    return anyOk;
}

// -----------------------------------------------------------------------------
// readMLX() - Canales 0/1/2 (extendidos, P82B715/Cat6)
// -----------------------------------------------------------------------------
bool SensorManager::readMLX(uint8_t channel, Adafruit_MLX90614 &mlx, bool &ready,
                             int16_t &outTempC_x100, uint16_t okFlag, TelemetryPayload &payload) {
    if (!_mux.selectChannel(channel, I2C_FREQ_EXTENDED_HZ)) {
        Serial.printf("[SensorManager] ERROR: no se pudo aislar Canal %d (MLX90614).\n", channel);
        return false;
    }

    if (!ready) {
        ready = mlx.begin(ADDR_MLX90614);
    }

    bool ok = false;
    if (ready) {
        // objectTempC es la temperatura del follaje medida sin contacto
        // (radiación IR); ambientTempC (no usada aquí) sería la del propio
        // sensor, útil solo para compensación interna del MLX90614.
        float tempC = mlx.readObjectTempC();

        if (!isnan(tempC)) {
            outTempC_x100 = Telemetry::packScaledInt16(tempC, TEMP_SCALE_FACTOR);
            payload.statusFlags |= okFlag;
            ok = true;
        } else {
            Serial.printf("[SensorManager] Lectura invalida (NaN) en Canal %d (MLX90614).\n", channel);
        }
    } else {
        Serial.printf("[SensorManager] ERROR: MLX90614 Canal %d no responde (begin fallo).\n", channel);
    }

    return ok;
}

// -----------------------------------------------------------------------------
// readBaseBME280() - Canal 3 (local)
// -----------------------------------------------------------------------------
bool SensorManager::readBaseBME280(TelemetryPayload &payload) {
    if (!_mux.selectChannel(CH_LOCAL_BME280, I2C_FREQ_LOCAL_HZ)) {
        Serial.println(F("[SensorManager] ERROR: no se pudo seleccionar Canal 3 (BME280 base)."));
        return false;
    }

    if (!_bmeBaseReady) {
        _bmeBaseReady = _bmeBase.begin(ADDR_BME280_LOCAL);
    }

    bool ok = false;
    if (_bmeBaseReady) {
        float temp = _bmeBase.readTemperature();
        float hum  = _bmeBase.readHumidity();
        float pres = _bmeBase.readPressure() / 100.0f;

        if (!isnan(temp) && !isnan(hum) && !isnan(pres)) {
            payload.tempBaseC_x100  = Telemetry::packScaledInt16(temp, TEMP_SCALE_FACTOR);
            payload.humBasePct_x100 = Telemetry::packScaledUInt16(hum, HUMIDITY_SCALE_FACTOR);
            payload.pressureHpa_x10 = Telemetry::packScaledUInt16(pres, PRESSURE_SCALE_FACTOR);
            payload.statusFlags |= FLAG_BASE_BME_OK;
            ok = true;
        } else {
            Serial.println(F("[SensorManager] Lectura invalida (NaN) del BME280 base."));
        }
    } else {
        Serial.println(F("[SensorManager] ERROR: BME280 base no responde (begin fallo)."));
    }

    return ok;
}

// -----------------------------------------------------------------------------
// readSoilMoisture() - ADC, no pasa por el mux I2C
// -----------------------------------------------------------------------------
bool SensorManager::readSoilMoisture(TelemetryPayload &payload) {
    int raw = analogRead(ADC_SOIL_MOISTURE_PIN);

    // Mapeo lineal entre los valores RAW calibrados en seco/húmedo
    // (config.h: SOIL_ADC_DRY_RAW/SOIL_ADC_WET_RAW son PLACEHOLDERS sin
    // calibrar). El sensor da un RAW más ALTO en seco y más BAJO en
    // húmedo, por eso el mapeo se invierte respecto a un map() directo.
    float pct = (static_cast<float>(SOIL_ADC_DRY_RAW - raw) /
                 static_cast<float>(SOIL_ADC_DRY_RAW - SOIL_ADC_WET_RAW)) * 100.0f;
    pct = constrain(pct, 0.0f, 100.0f);

    payload.soilMoisturePct_x100 = Telemetry::packScaledUInt16(pct, SOIL_MOISTURE_SCALE_FACTOR);
    payload.statusFlags |= FLAG_SOIL_OK;
    return true;
}

// -----------------------------------------------------------------------------
// readBatteryVoltage() - ADC, no pasa por el mux I2C
// -----------------------------------------------------------------------------
void SensorManager::readBatteryVoltage(TelemetryPayload &payload) {
    // analogReadMilliVolts() aplica la curva de calibración de fábrica del
    // ADC del ESP32-S3 (eFuse), corrigiendo la no linealidad cerca de los
    // extremos del rango en vez de una lectura RAW sin corregir.
    uint32_t pinMilliVolts = analogReadMilliVolts(ADC_BATTERY_PIN);
    payload.batteryMilliVolts = static_cast<uint16_t>(pinMilliVolts * BATTERY_DIVIDER_RATIO);

    // Umbral de batería baja: 3.4V es un punto razonable de corte para
    // Li-Ion antes de la zona de descarga profunda, pero no está
    // formalizado como constante de config.h todavía — placeholder.
    constexpr uint16_t LOW_BATTERY_MV = 3400;
    if (payload.batteryMilliVolts > 0 && payload.batteryMilliVolts < LOW_BATTERY_MV) {
        payload.statusFlags |= FLAG_LOW_BATTERY;
    }
}
