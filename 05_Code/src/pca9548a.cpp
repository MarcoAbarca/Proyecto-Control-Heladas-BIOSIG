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

bool PCA9548A::begin() {
    Wire.beginTransmission(_address);
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.printf("[PCA9548A] ERROR: sin respuesta en 0x%02X (err=%d)\n", _address, error);
        return false;
    }

    disableAll();
    Serial.printf("[PCA9548A] Multiplexor detectado en 0x%02X, canales cerrados.\n", _address);
    return true;
}

bool PCA9548A::selectChannel(uint8_t channel, uint32_t frequencyHz) {
    if (channel >= PCA9548A_CHANNEL_COUNT) {
        Serial.printf("[PCA9548A] Canal %d invalido (rango 0-7).\n", channel);
        return false;
    }

    // La frecuencia se ajusta ANTES de mandar el comando de selección: si
    // el tramo de cable de este canal no soporta 400kHz, el propio comando
    // de selección podria fallar si se enviara a esa velocidad.
    I2CBus::setFrequency(frequencyHz);

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

void PCA9548A::disableAll() {
    writeChannelMask(0x00);
    _activeChannel = NO_CHANNEL;
    I2CBus::setFrequency(I2C_FREQ_LOCAL_HZ);
}

int8_t PCA9548A::getActiveChannel() const {
    return _activeChannel;
}

bool PCA9548A::writeChannelMask(uint8_t mask) {
    Wire.beginTransmission(_address);
    Wire.write(mask);
    return Wire.endTransmission() == 0;
}

bool PCA9548A::readChannelMask(uint8_t &outMask) {
    uint8_t received = Wire.requestFrom(_address, static_cast<uint8_t>(1));
    if (received != 1) {
        return false;
    }
    outMask = Wire.read();
    return true;
}
