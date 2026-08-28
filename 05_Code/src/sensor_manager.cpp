/**
 * =============================================================================
 * sensor_manager.cpp
 * =============================================================================
 */

#include "sensor_manager.h"
#include "config.h"
#include "i2c_bus.h"

SensorManager::SensorManager(PCA9548A &mux) : _mux(mux) {}

// readChannel()
SensorReading SensorManager::readChannel(uint8_t channel) {
    if (!_mux.selectChannel(channel)) {
        SensorReading r;
        r.channel = channel;
        r.status = SensorStatus::MUX_ERROR;
        r.timestampMs = millis();
        return r;
    }

    return readWithTimeout(channel, SENSOR_READ_TIMEOUT_MS);
}

// -----------------------------------------------------------------------------
// readExtendedChannel()
// -----------------------------------------------------------------------------
SensorReading SensorManager::readExtendedChannel() {
    if (!_mux.selectExtendedChannel()) {
        SensorReading r;
        r.channel = CH_EXT_LONGBUS;
        r.status = SensorStatus::MUX_ERROR;
        r.timestampMs = millis();
        return r;
    }

    SensorReading reading = readWithTimeout(CH_EXT_LONGBUS, SENSOR_READ_TIMEOUT_MS);

    // Fundamental: siempre liberar el canal extendido y restaurar la
    // frecuencia local al terminar, sin importar si la lectura fue
    // exitosa o no. Si esto se omitiera en una rama de error temprana,
    // el bus quedaría atrapado a 100kHz de forma silenciosa.
    _mux.releaseExtendedChannel();

    return reading;
}

// -----------------------------------------------------------------------------
// readWithTimeout() [privado]
// -----------------------------------------------------------------------------
SensorReading SensorManager::readWithTimeout(uint8_t channel, uint32_t timeoutMs) {
    SensorReading reading;
    reading.channel = channel;

    const unsigned long start = millis();

    // ---------------------------------------------------------------
    // TODO: lectura real del sensor.
    //
    // Cuando se integre un driver concreto (BME280/SHT3x/MPU6050/etc.),
    // su espera de conversión debe expresarse dentro de este mismo
    // patrón, por ejemplo:
    //
    //   while (!sensor.dataReady()) {
    //       if (millis() - start >= timeoutMs) {
    //           reading.status = SensorStatus::TIMEOUT;
    //           reading.timestampMs = millis();
    //           return reading;
    //       }
    //       // sin delay() aquí: el loop() principal sigue libre para
    //       // atender otras tareas mientras se espera al sensor.
    //   }
    //   reading.value1 = sensor.readTemperature();
    //   reading.status = SensorStatus::OK;
    //
    // Por ahora, sin driver asignado, se reporta explícitamente el
    // estado para que el log de campo no confunda "sin implementar"
    // con una falla real de hardware.
    // ---------------------------------------------------------------
    (void)start;
    (void)timeoutMs;

    reading.status = SensorStatus::NOT_IMPLEMENTED;
    reading.timestampMs = millis();
    return reading;
}
