/**
 * =============================================================================
 * pca9548a.h
 * -----------------------------------------------------------------------------
 * Clase de control del multiplexor I2C PCA9548A.
 *
 * Principio de diseño clave: AISLAMIENTO ESTRICTO DE CANAL. El registro de
 * control del PCA9548A permite, en teoría, activar varios canales a la vez
 * (OR de bits), pero este proyecto tiene sensores idénticos repetidos en
 * distintos canales (ej. BME280 en Canal 1 y BME280 en Canal 0 externo).
 * Si dos canales con la misma dirección quedaran activos simultáneamente,
 * ambos responderían a la vez y corromperían la lectura I2C. Por eso esta
 * clase NUNCA escribe una máscara con más de un bit encendido: cada llamada
 * a selectChannel() desactiva implícitamente cualquier canal anterior.
 *
 * Además, cada selección se verifica con una lectura de retorno (readback)
 * del registro de control, para detectar de inmediato si el propio
 * multiplexor dejó de responder (útil para distinguir "el sensor falló" de
 * "el multiplexor falló", que son problemas de diagnóstico distintos).
 * =============================================================================
 */

#ifndef PCA9548A_H
#define PCA9548A_H

#include <Arduino.h>

class PCA9548A {
public:
    // Sin canal activo. Usado como valor de retorno de getActiveChannel()
    // cuando disableAll() fue la última operación realizada.
    static constexpr int8_t NO_CHANNEL = -1;

    explicit PCA9548A(uint8_t i2cAddress);

    /**
     * Verifica que el multiplexor responda en el bus (ACK simple a su
     * dirección) y lo deja en estado "todos los canales cerrados".
     * Debe llamarse una vez en setup(), después de I2CBus::begin().
     *
     * @return true si el PCA9548A respondió correctamente.
     */
    bool begin();

    /**
     * Selecciona un único canal (0-7) de forma exclusiva: cualquier canal
     * previamente activo queda desactivado por la misma escritura, ya que
     * la máscara enviada tiene un solo bit encendido.
     *
     * Verifica el éxito de la operación releyendo el registro de control
     * del PCA9548A y comparándolo con la máscara esperada.
     *
     * @param channel Canal 0-7.
     * @return true si la selección fue escrita y confirmada por readback.
     */
    bool selectChannel(uint8_t channel);

    /**
     * Atajo para aislar específicamente el Canal 0 (extensor P82B715 /
     * bus largo Cat6). Además de seleccionar el canal, ajusta la
     * frecuencia del bus a I2C_FREQ_EXTENDED_HZ (ver config.h), ya que
     * operar el cable largo a 400kHz puede producir errores por la
     * capacitancia parásita del cable.
     *
     * @return true si el canal quedó correctamente aislado.
     */
    bool selectExtendedChannel();

    /**
     * Cierra el canal extendido y restaura la frecuencia local
     * (I2C_FREQ_LOCAL_HZ). Debe llamarse siempre después de terminar de
     * leer los sensores externos, antes de volver a operar canales
     * locales, para no arrastrar la frecuencia reducida sin necesidad.
     */
    void releaseExtendedChannel();

    /**
     * Desactiva todos los canales (máscara 0x00). Es el estado seguro por
     * defecto: se usa antes de entrar en Deep Sleep y también como
     * limpieza defensiva entre lecturas de sensores en canales distintos.
     */
    void disableAll();

    /**
     * @return El canal actualmente activo (0-7), o NO_CHANNEL si no hay
     *         ninguno seleccionado (estado tras disableAll() o tras un
     *         begin() recién ejecutado).
     */
    int8_t getActiveChannel() const;

private:
    uint8_t _address;
    int8_t  _activeChannel;

    // Escribe la máscara cruda al registro de control del PCA9548A.
    bool writeChannelMask(uint8_t mask);

    // Lee de vuelta el registro de control actual del PCA9548A, para
    // confirmar que la última escritura fue efectiva.
    bool readChannelMask(uint8_t &outMask);
};

#endif // PCA9548A_H
