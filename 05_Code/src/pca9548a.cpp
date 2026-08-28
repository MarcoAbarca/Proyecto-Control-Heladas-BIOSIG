/**
 * =============================================================================
 * pca9548a.cpp
 * =============================================================================
 */

#include "pca9548a.h"
#include "config.h"
#include "i2c_bus.h"
#include <Wire.h>

PCA9548A::PCA9548A(uint8_t i2cAddress)
    : _address(i2cAddress), _activeChannel(NO_CHANNEL) {}

// -----------------------------------------------------------------------------
// begin()
// -----------------------------------------------------------------------------
bool PCA9548A::begin() {
    Wire.beginTransmission(_address);
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.printf("[PCA9548A] ERROR: sin respuesta en 0x%02X (err=%d)\n", _address, error);
        return false;
    }

    // Estado inicial seguro: todos los canales cerrados hasta que algo
    // los solicite explícitamente.
    disableAll();
    Serial.printf("[PCA9548A] Multiplexor detectado en 0x%02X, canales cerrados.\n", _address);
    return true;
}

// -----------------------------------------------------------------------------
// selectChannel()
// -----------------------------------------------------------------------------
bool PCA9548A::selectChannel(uint8_t channel) {
    if (channel >= PCA9548A_CHANNEL_COUNT) {
        Serial.printf("[PCA9548A] Canal %d invalido (rango 0-7).\n", channel);
        return false;
    }

    // Máscara de un solo bit: garantiza aislamiento, nunca se combinan
    // canales (ver justificación de diseño en pca9548a.h).
    const uint8_t expectedMask = static_cast<uint8_t>(1 << channel);

    if (!writeChannelMask(expectedMask)) {
        Serial.printf("[PCA9548A] ERROR al escribir seleccion de canal %d.\n", channel);
        return false;
    }

    uint8_t readback = 0;
    if (!readChannelMask(readback)) {
        Serial.printf("[PCA9548A] ERROR: sin respuesta al confirmar canal %d (readback).\n", channel);
        return false;
    }

    if (readback != expectedMask) {
        Serial.printf(
            "[PCA9548A] ERROR: readback (0x%02X) no coincide con lo escrito (0x%02X) en canal %d.\n",
            readback, expectedMask, channel
        );
        return false;
    }

    _activeChannel = static_cast<int8_t>(channel);
    return true;
}

// -----------------------------------------------------------------------------
// selectExtendedChannel()
// -----------------------------------------------------------------------------
bool PCA9548A::selectExtendedChannel() {
    // Se baja la frecuencia ANTES de seleccionar el canal: si el propio
    // comando de selección ya viaja a 400kHz sobre un bus con capacitancia
    // límite, podría fallar el readback de confirmación.
    I2CBus::setFrequency(I2C_FREQ_EXTENDED_HZ);

    bool ok = selectChannel(CH_EXT_LONGBUS);
    if (!ok) {
        Serial.println(F("[PCA9548A] Fallo al aislar Canal 0 (extensor P82B715)."));
    }
    return ok;
}

// -----------------------------------------------------------------------------
// releaseExtendedChannel()
// -----------------------------------------------------------------------------
void PCA9548A::releaseExtendedChannel() {
    disableAll();
    I2CBus::setFrequency(I2C_FREQ_LOCAL_HZ);
}

// -----------------------------------------------------------------------------
// disableAll()
// -----------------------------------------------------------------------------
void PCA9548A::disableAll() {
    writeChannelMask(0x00);
    _activeChannel = NO_CHANNEL;
}

// -----------------------------------------------------------------------------
// getActiveChannel()
// -----------------------------------------------------------------------------
int8_t PCA9548A::getActiveChannel() const {
    return _activeChannel;
}

// -----------------------------------------------------------------------------
// writeChannelMask() [privado]
// -----------------------------------------------------------------------------
bool PCA9548A::writeChannelMask(uint8_t mask) {
    Wire.beginTransmission(_address);
    Wire.write(mask);
    return Wire.endTransmission() == 0;
}

// -----------------------------------------------------------------------------
// readChannelMask() [privado]
// -----------------------------------------------------------------------------
bool PCA9548A::readChannelMask(uint8_t &outMask) {
    // El PCA9548A devuelve su registro de control actual ante un simple
    // requestFrom de 1 byte, sin necesidad de escribir un puntero de
    // registro previo (no tiene direccionamiento interno de registros).
    uint8_t received = Wire.requestFrom(_address, static_cast<uint8_t>(1));
    if (received != 1) {
        return false;
    }
    outMask = Wire.read();
    return true;
}
