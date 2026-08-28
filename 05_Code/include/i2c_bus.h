/**
 * =============================================================================
 * i2c_bus.h
 * -----------------------------------------------------------------------------
 * Módulo de bajo nivel para el bus I2C principal (D4/D5 -> PCA9548A).
 *
 * Responsabilidades:
 *   1. Inicializar el bus con la frecuencia adecuada.
 *   2. Detectar y recuperar un bus "colgado" (SDA/SCL atascadas en LOW),
 *      típico tras un corte de energía a mitad de una transacción.
 *   3. Escanear direcciones I2C, tanto en el bus raíz como canal por canal
 *      a través del multiplexor PCA9548A (selección "cruda" por registro,
 *      sin depender aún de la clase PCA9548A que se implementa en Fase 3).
 *
 * Este módulo NO conoce sensores específicos más allá de poder nombrarlos
 * en el log de diagnóstico (usando las direcciones declaradas en config.h).
 * =============================================================================
 */

#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <Arduino.h>
#include <Wire.h>

namespace I2CBus {

    /**
     * Inicializa el bus I2C principal en los pines definidos en config.h,
     * a la frecuencia "local" (400kHz) por defecto. Debe llamarse una sola
     * vez en setup() antes de cualquier otra operación I2C.
     */
    void begin();

    /**
     * Cambia la frecuencia de operación del bus en caliente. Se usa para
     * bajar a 100kHz justo antes de operar sobre el Canal 0 (P82B715 +
     * Cat6) y volver a 400kHz al terminar, sin reiniciar todo el bus.
     */
    void setFrequency(uint32_t freqHz);

    /**
     * Rutina de recuperación de bus (I2C Bus Recovery).
     *
     * Se dispara cuando una transacción falla de forma persistente
     * (posible SDA atascada en LOW porque un esclavo quedó a mitad de
     * byte esperando un flanco de reloj que nunca llegó). Libera
     * temporalmente los pines del control del periférico Wire, los
     * maneja como GPIO puro, y envía hasta I2C_RECOVERY_CLOCK_PULSES
     * pulsos manuales por SCL para forzar al esclavo a soltar SDA,
     * seguido de una condición STOP manual. Al finalizar, reinicializa
     * el periférico Wire normalmente.
     *
     * @return true si el bus quedó liberado (SDA en HIGH al terminar),
     *         false si SDA sigue atascada (posible falla de hardware).
     */
    bool recoverBus();

    /**
     * Verifica de forma rápida si el bus está atascado, leyendo el
     * estado de SDA como GPIO antes de reiniciar Wire. Útil para decidir
     * si vale la pena invocar recoverBus() sin pagar el costo de la
     * rutina completa en cada ciclo.
     */
    bool isBusStuck();

    /**
     * Retorna un nombre descriptivo para una dirección I2C conocida
     * (definida en config.h), o "Desconocido" si no coincide con ningún
     * sensor previsto en el proyecto. Solo para fines de logging.
     */
    const char* identifyDevice(uint8_t address);

    /**
     * Escanea el bus I2C actualmente activo (direcciones 0x08 a 0x77) e
     * imprime por Serial cada dispositivo encontrado junto con su nombre
     * identificado (si corresponde). Retorna la cantidad de dispositivos
     * hallados.
     */
    uint8_t scanBus();

    /**
     * Selección "cruda" de canal en el PCA9548A, escribiendo directamente
     * el registro de control (1 byte, bit N = canal N). Se usa solo en
     * esta fase de diagnóstico; en Fase 3 esta responsabilidad pasa a la
     * clase PCA9548A dedicada.
     *
     * @param channel Canal 0-7. Un valor fuera de rango no hace nada.
     */
    void selectMuxChannelRaw(uint8_t channel);

    /**
     * Cierra todos los canales del multiplexor (registro de control en
     * 0x00), dejando el bus aislado. Debe llamarse siempre después de
     * terminar de operar un canal, para evitar colisiones de direcciones
     * repetidas entre sensores idénticos en distintos canales.
     */
    void closeAllMuxChannels();

    /**
     * Recorre los 8 canales del PCA9548A (0 a 7): selecciona el canal,
     * ajusta la frecuencia apropiada (100kHz si es el Canal 0 extendido,
     * 400kHz en el resto), ejecuta scanBus() y cierra el canal antes de
     * continuar con el siguiente. Pensado como diagnóstico de arranque
     * (Fase 1) para verificar cableado completo del sistema.
     */
    void scanAllMuxChannels();

} // namespace I2CBus

#endif // I2C_BUS_H
