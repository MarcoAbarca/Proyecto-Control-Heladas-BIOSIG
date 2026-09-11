/**
 * =============================================================================
 * sensor_manager.cpp
 * =============================================================================
 */

#include "sensor_manager.h"
#include "config.h"
#include "i2c_bus.h"

SensorManager::SensorManager(PCA9548A &mux)
    : _mux(mux), _soilOneWire(SOIL_TEMPERATURE_DATA_PIN), _soilTemperature(&_soilOneWire) {
    _soilTemperature.begin();
}

// -----------------------------------------------------------------------------
// readAllInto()
// -----------------------------------------------------------------------------
bool SensorManager::readAllInto(TelemetryPayload &payload) {
    bool anyI2COk = false;

    // Variables LOCALES (no referencias a campos packed) — ver nota en
    // sensor_manager.h sobre por qué readMLX() no escribe directo al struct.
    int16_t tempTop = 0, tempMid = 0, tempLow = 0;

    if (readMLX(CH_MLX_TOP, _mlxTop, _mlxTopReady, tempTop, FLAG_MLX_TOP_OK, payload)) {
        payload.tempCanopyTop_x100 = tempTop;
        anyI2COk = true;
    }
    if (readMLX(CH_MLX_MID, _mlxMid, _mlxMidReady, tempMid, FLAG_MLX_MID_OK, payload)) {
        payload.tempCanopyMid_x100 = tempMid;
        anyI2COk = true;
    }
    if (readMLX(CH_MLX_LOW, _mlxLow, _mlxLowReady, tempLow, FLAG_MLX_LOW_OK, payload)) {
        payload.tempCanopyLow_x100 = tempLow;
        anyI2COk = true;
    }

    anyI2COk |= readBaseBME280(payload);

    _mux.disableAll(); // Estado seguro: cierra canal I2C antes de pasar al ADC.

    readSoilMoisture(payload);
    readSoilTemperature(payload);

    return anyI2COk;
}

// -----------------------------------------------------------------------------
// readSoilOnly() - camino rápido cuando el mux falló
// -----------------------------------------------------------------------------
bool SensorManager::readSoilOnly(TelemetryPayload &payload) {
    readSoilMoisture(payload);
    readSoilTemperature(payload);
    return false;
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
    if (!SOIL_MOISTURE_ADC_AVAILABLE) {
        payload.soilMoisturePct_x100 = 0;
        payload.statusFlags &= static_cast<uint16_t>(~FLAG_SOIL_OK);
        return false;
    }

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
// readSoilTemperature() - DS18B20 por 1-Wire en D7
// -----------------------------------------------------------------------------
bool SensorManager::readSoilTemperature(TelemetryPayload &payload) {
    _soilTemperature.requestTemperatures();
    const float tempC = _soilTemperature.getTempCByIndex(0);

    if (tempC == DEVICE_DISCONNECTED_C || isnan(tempC)) {
        payload.tempSoilC_x100 = 0;
        payload.statusFlags &= static_cast<uint16_t>(~FLAG_SOIL_TEMP_OK);
        Serial.println(F("[SensorManager] ERROR: DS18B20 de suelo no responde."));
        return false;
    }

    payload.tempSoilC_x100 = Telemetry::packScaledInt16(tempC, TEMP_SCALE_FACTOR);
    payload.statusFlags |= FLAG_SOIL_TEMP_OK;
    return true;
}

