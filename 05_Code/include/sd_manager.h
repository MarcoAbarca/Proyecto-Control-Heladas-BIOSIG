/**
 * =============================================================================
 * sd_manager.h
 * -----------------------------------------------------------------------------
 * Respaldo local en MicroSD (formato CSV) de la telemetría recibida por
 * LoRa P2P. Exclusivo del rol RECEPTOR — este archivo se compila solo
 * cuando NODE_ROLE_RECEPTOR está definido (ver platformio.ini).
 *
 * Diseño: un archivo CSV por día ("/telemetria_YYYYMMDD.csv"), con
 * encabezado escrito una sola vez. Cada TelemetryPayload recibido se
 * desempaqueta (usando los mismos factores de escala de telemetry.h) y
 * se agrega como una fila. Preparado para que, en el futuro, el mismo
 * struct desempaquetado se use también para insertar en una base SQL,
 * sin duplicar la lógica de conversión.
 * =============================================================================
 */

#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#ifdef NODE_ROLE_RECEPTOR

#include <Arduino.h>
#include <SdFat.h>
#include "telemetry.h"

class SDManager {
public:
    /**
     * Inicializa el bus SPI compartido y monta la tarjeta MicroSD en
     * SD_CS_PIN (config.h). Debe llamarse una sola vez en setup(),
     * después de que el radio LoRa ya haya inicializado el mismo bus
     * SPI (comparten SCK/MISO/MOSI; cada uno tiene su propio CS).
     *
     * @return true si la tarjeta se montó correctamente.
     */
    bool begin();

    /**
     * Escribe una fila de telemetría en el archivo CSV del día. Si el
     * archivo del día no existe todavía, lo crea con encabezado. Convierte
     * los campos escalados de TelemetryPayload a sus valores físicos
     * reales (float) antes de escribir, usando los mismos factores que
     * usó el Emisor para empaquetar (telemetry.h), así el CSV queda en
     * unidades legibles (°C, %RH, hPa, mg, °/s) en vez de enteros crudos.
     *
     * No bloqueante en el sentido de la FSM: la propia operación de
     * escritura en SD es breve y se ejecuta como un solo paso discreto
     * del ciclo RECEIVE -> LOG_SD -> BRIDGE_TTN de la FSM del Receptor;
     * no hay una espera activa aquí.
     *
     * @return true si la fila se escribió y el archivo se cerró (flush)
     *         correctamente.
     */
    bool logPayload(const TelemetryPayload &payload);

    /**
     * @return true si la última operación de SD (begin o logPayload)
     *         fue exitosa. Útil para que la FSM del Receptor decida si
     *         debe reintentar el montaje de la tarjeta.
     */
    bool isReady() const;

private:
    SdFat _sd;
    bool  _ready = false;

    // Nombre de archivo del día actual, cacheado para no reconstruirlo
    // en cada lectura (se recalcula solo cuando cambia el día, una vez
    // que haya una fuente de fecha real - ver nota en el .cpp).
    String _currentFilename;

    String buildFilenameForToday() const;
    bool   writeHeaderIfNewFile(const String &filename);
};

#endif // NODE_ROLE_RECEPTOR

#endif // SD_MANAGER_H
