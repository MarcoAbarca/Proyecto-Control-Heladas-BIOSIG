/**
 * =============================================================================
 * sensor_manager.h
 * -----------------------------------------------------------------------------
 * Interfaz GENÉRICA para la lectura de sensores por canal del PCA9548A.
 *
 * IMPORTANTE: esta fase entrega el ESQUELETO del orquestador, sin drivers
 * de sensores concretos todavía (BME280 / SHT3x / MPU6050 se integran más
 * adelante, cuando se defina qué sensores van en qué canal). Por eso
 * readChannel() y readExtendedChannel() devuelven SensorStatus::NOT_IMPLEMENTED:
 * el objetivo de este archivo es fijar el CONTRATO (qué recibe, qué
 * devuelve, cómo se manejan timeouts) para que integrar un driver real
 * después sea reemplazar el cuerpo de una función, no rediseñar la FSM.
 *
 * Patrón de timeout no bloqueante: readWithTimeout() muestra cómo debe
 * envolverse cualquier lectura real (ej. tiempo de conversión de un
 * sensor) usando millis() en vez de delay(), para que un sensor colgado
 * nunca detenga el loop() principal.
 * =============================================================================
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "pca9548a.h"

enum class SensorStatus {
    OK,               // Lectura válida.
    TIMEOUT,          // El sensor no respondió dentro de SENSOR_READ_TIMEOUT_MS.
    NO_ACK,           // El canal no tiene ningún dispositivo respondiendo (I2C NACK).
    MUX_ERROR,        // Falló la selección del canal en el PCA9548A.
    NOT_IMPLEMENTED   // Aún no hay driver real asignado a este canal (fase actual).
};

struct SensorReading {
    uint8_t channel = 0;
    SensorStatus status = SensorStatus::NOT_IMPLEMENTED;

    // Valores genéricos: hasta que se asignen drivers reales, cada sensor
    // decidirá qué representa value1/2/3 (ej. BME280: temp/hum/presión;
    // MPU6050: iría en una estructura aparte por tener 6 ejes).
    float value1 = NAN;
    float value2 = NAN;
    float value3 = NAN;

    unsigned long timestampMs = 0;
};

class SensorManager {
public:
    explicit SensorManager(PCA9548A &mux);

    /**
     * Lee un canal LOCAL (1-7). Selecciona el canal en el multiplexor y
     * delega en el driver correspondiente (a integrar en una fase
     * posterior). Por ahora retorna NOT_IMPLEMENTED si la selección de
     * canal fue exitosa, o MUX_ERROR si no.
     */
    SensorReading readChannel(uint8_t channel);

    /**
     * Lee el Canal 0 (extensor P82B715 / bus largo Cat6). Usa
     * mux.selectExtendedChannel()/releaseExtendedChannel() para aplicar
     * automáticamente la frecuencia reducida durante la lectura.
     */
    SensorReading readExtendedChannel();

private:
    PCA9548A &_mux;

    /**
     * Plantilla del patrón de timeout no bloqueante. Un driver real debe
     * reemplazar el cuerpo (la sección "TODO: lectura real del sensor")
     * conservando el chequeo de millis() para no dejar nunca un
     * while(true) o delay() esperando al sensor.
     */
    SensorReading readWithTimeout(uint8_t channel, uint32_t timeoutMs);
};

#endif // SENSOR_MANAGER_H
