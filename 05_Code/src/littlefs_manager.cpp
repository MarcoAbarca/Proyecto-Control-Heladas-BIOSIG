/**
 * =============================================================================
 * littlefs_manager.cpp
 * =============================================================================
 */

#include "littlefs_manager.h"

#ifdef NODE_ROLE_EMISOR

#include <LittleFS.h>

bool LittleFSManager::begin() {
    // 'true' = formatear automáticamente si el montaje falla (primer
    // arranque / flash corrupta). En un nodo de campo sin acceso físico
    // frecuente, preferimos un FS funcional aunque vacío antes que un
    // nodo que deja de respaldar datos silenciosamente por un mount fallido.
    _ready = LittleFS.begin(true);

    if (!_ready) {
        Serial.println(F("[LittleFSManager] ERROR: no se pudo montar/formatear LittleFS."));
        return false;
    }

    Serial.println(F("[LittleFSManager] LittleFS montado correctamente."));
    return true;
}

bool LittleFSManager::isReady() const {
    return _ready;
}

bool LittleFSManager::writeHeaderIfNewFile() {
    if (LittleFS.exists(LOG_FILENAME)) {
        return true;
    }

    File file = LittleFS.open(LOG_FILENAME, "w");
    if (!file) {
        Serial.println(F("[LittleFSManager] ERROR: no se pudo crear el archivo CSV."));
        return false;
    }

    file.println(F(
        "nodeId,sequenceNumber,tempCanopyTopC,tempCanopyMidC,tempCanopyLowC,"
        "tempBaseC,humBasePct,pressureHpa,soilMoisturePct,batteryMilliVolts,statusFlags"
    ));
    file.close();
    return true;
}

bool LittleFSManager::logPayload(const TelemetryPayload &payload) {
    if (!_ready) {
        Serial.println(F("[LittleFSManager] ERROR: LittleFS no montado, se descarta el registro."));
        return false;
    }

    if (!writeHeaderIfNewFile()) {
        return false;
    }

    File file = LittleFS.open(LOG_FILENAME, "a");
    if (!file) {
        Serial.println(F("[LittleFSManager] ERROR: no se pudo abrir el CSV para escritura."));
        return false;
    }

    file.print(payload.nodeId);                  file.print(',');
    file.print(payload.sequenceNumber);           file.print(',');
    file.print(Telemetry::unpackScaled(payload.tempCanopyTop_x100, TEMP_SCALE_FACTOR), 2); file.print(',');
    file.print(Telemetry::unpackScaled(payload.tempCanopyMid_x100, TEMP_SCALE_FACTOR), 2); file.print(',');
    file.print(Telemetry::unpackScaled(payload.tempCanopyLow_x100, TEMP_SCALE_FACTOR), 2); file.print(',');
    file.print(Telemetry::unpackScaled(payload.tempBaseC_x100, TEMP_SCALE_FACTOR), 2);     file.print(',');
    file.print(Telemetry::unpackScaled(payload.humBasePct_x100, HUMIDITY_SCALE_FACTOR), 2);file.print(',');
    file.print(Telemetry::unpackScaled(payload.pressureHpa_x10, PRESSURE_SCALE_FACTOR), 1); file.print(',');
    file.print(Telemetry::unpackScaled(payload.soilMoisturePct_x100, SOIL_MOISTURE_SCALE_FACTOR), 2); file.print(',');
    file.print(payload.batteryMilliVolts);         file.print(',');
    file.println(payload.statusFlags, BIN);

    file.close(); // Flush explícito: crítico antes de un Deep Sleep inminente.
    return true;
}

void LittleFSManager::logStorageUsage() const {
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    Serial.printf("[LittleFSManager] Uso de flash: %u / %u bytes (%.1f%%)\n",
                  (unsigned)used, (unsigned)total, (used * 100.0f) / total);
}

#endif // NODE_ROLE_EMISOR
