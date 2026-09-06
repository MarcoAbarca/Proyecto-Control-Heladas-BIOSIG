/**
 * =============================================================================
 * littlefs_manager.h
 * -----------------------------------------------------------------------------
 * Respaldo local en la flash interna del XIAO ESP32-S3 (LittleFS, 8MB),
 * exclusivo del rol EMISOR. Existe para no depender del bus SPI (ya
 * ocupado por la radio) para guardar un respaldo — LittleFS vive en la
 * flash SPI *interna* del propio módulo, no en un periférico externo.
 *
 * Sigue el mismo patrón de sd_manager.h (Receptor): un CSV con
 * encabezado escrito una sola vez, filas desempaquetadas a unidades
 * físicas reales usando los factores de escala de telemetry.h.
 * =============================================================================
 */

#ifndef LITTLEFS_MANAGER_H
#define LITTLEFS_MANAGER_H

#ifdef NODE_ROLE_EMISOR

#include <Arduino.h>
#include "telemetry.h"

class LittleFSManager {
public:
    /**
     * Monta LittleFS. Si el sistema de archivos no existe todavía (primer
     * arranque del nodo), lo formatea automáticamente — comportamiento
     * estándar de LittleFS en Arduino-ESP32, aceptable acá porque no hay
     * datos previos que proteger en un flasheo inicial.
     */
    bool begin();

    /**
     * Agrega una fila con la lectura de este ciclo al CSV de respaldo.
     * A diferencia de sd_manager (que corre en un nodo con alimentación
     * continua), esta llamada ocurre justo antes de un Deep Sleep, así
     * que debe ser rápida y cerrar el archivo (flush) antes de retornar,
     * sin dejar nada pendiente que dependa de una vuelta futura de loop().
     */
    bool logPayload(const TelemetryPayload &payload);

    bool isReady() const;

    /**
     * Uso aproximado de la flash (bytes usados / totales). Útil para que
     * la FSM decida si conviene rotar o limpiar el archivo antes de que
     * los 8MB se llenen — con 15 min/ciclo y ~30 bytes de fila CSV, la
     * capacidad alcanza para años, pero se deja el gancho para el futuro.
     */
    void logStorageUsage() const;

private:
    bool _ready = false;
    static constexpr const char* LOG_FILENAME = "/telemetria_local.csv";

    bool writeHeaderIfNewFile();
};

#endif // NODE_ROLE_EMISOR

#endif // LITTLEFS_MANAGER_H
