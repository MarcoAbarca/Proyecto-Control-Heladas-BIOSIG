/**
 * =============================================================================
 * pca9548a.h
 * -----------------------------------------------------------------------------
 * ACTUALIZADO: generalizado para soportar frecuencia configurable por
 * canal. Antes solo el Canal 0 tenía tratamiento "extendido" (100kHz);
 * ahora son 3 canales (0, 1, 2 - los tres MLX90614 vía P82B715), cada uno
 * con su propio cable Cat6, y el Canal 3 (BME280 local) sigue a 400kHz.
 *
 * Se mantiene el principio de aislamiento estricto: selectChannel() nunca
 * escribe una máscara con más de un bit encendido, sin importar cuántos
 * canales usen la misma dirección I2C (los 3 MLX90614 comparten 0x5A).
 * =============================================================================
 */

#ifndef PCA9548A_H
#define PCA9548A_H

#include <Arduino.h>

class PCA9548A {
public:
    static constexpr int8_t NO_CHANNEL = -1;

    explicit PCA9548A(uint8_t i2cAddress);

    bool begin();

    /**
     * Selecciona un único canal (0-7) de forma exclusiva y ajusta la
     * frecuencia del bus I2C a `frequencyHz` ANTES de aislar el canal
     * (evita que el propio comando de selección viaje a una frecuencia
     * que el tramo de cable en cuestión no soporte). Verifica el éxito
     * por readback del registro de control del PCA9548A.
     *
     * Reemplaza el par selectExtendedChannel()/selectChannel() de la
     * fase anterior: ahora CUALQUIER canal puede pedir su propia
     * frecuencia, no solo un canal hardcodeado.
     *
     * @param channel Canal 0-7.
     * @param frequencyHz Frecuencia I2C a usar para este canal (ver
     *        I2C_FREQ_LOCAL_HZ / I2C_FREQ_EXTENDED_HZ en config.h).
     * @return true si la selección fue escrita y confirmada por readback.
     */
    bool selectChannel(uint8_t channel, uint32_t frequencyHz);

    /**
     * Desactiva todos los canales (máscara 0x00) y restaura la frecuencia
     * a I2C_FREQ_LOCAL_HZ como estado seguro por defecto. Debe llamarse
     * siempre después de terminar de operar un canal.
     */
    void disableAll();

    int8_t getActiveChannel() const;

private:
    uint8_t _address;
    int8_t  _activeChannel;

    bool writeChannelMask(uint8_t mask);
    bool readChannelMask(uint8_t &outMask);
};

#endif // PCA9548A_H
