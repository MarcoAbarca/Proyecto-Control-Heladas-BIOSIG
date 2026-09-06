/**
 * =============================================================================
 * sd_manager.cpp
 * =============================================================================
 */

#include "sd_manager.h"

#ifdef NODE_ROLE_RECEPTOR

#include "config.h"

// -----------------------------------------------------------------------------
// begin()
// -----------------------------------------------------------------------------
bool SDManager::begin() {
    // SPI_SCK/MISO/MOSI ya deben estar inicializados por lora_manager antes
    // de esta llamada (bus compartido). Aquí solo se agrega el CS de la SD
    // y se monta el sistema de archivos.
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH); // Deseleccionado por defecto

    _ready = _sd.begin(SD_CS_PIN, SD_SCK_MHZ(20)); // 20MHz: conservador para
                                                     // un bus compartido con
                                                     // el radio LoRa.

    if (!_ready) {
        Serial.println(F("[SDManager] ERROR: no se pudo montar la MicroSD."));
        return false;
    }

    Serial.println(F("[SDManager] MicroSD montada correctamente."));
    return true;
}

// -----------------------------------------------------------------------------
// isReady()
// -----------------------------------------------------------------------------
bool SDManager::isReady() const {
    return _ready;
}

// -----------------------------------------------------------------------------
// buildFilenameForToday()
// -----------------------------------------------------------------------------
String SDManager::buildFilenameForToday() const {
    // LIMITACIÓN CONOCIDA: el Receptor todavía no tiene fuente de fecha real
    // (ni RTC ni NTP vía WiFi). Sin eso, "el archivo del día" no puede
    // rotar por calendario real. Por ahora se usa un único archivo
    // acumulativo por nodo, y se deja este método como el punto exacto
    // donde conectar una fecha real (ej. leyendo un RTC DS3231, o NTP si
    // el Receptor tiene WiFi disponible) sin tocar el resto de la clase.
    return String(SD_LOG_FILENAME_PREFIX) + "acumulado.csv";
}

// -----------------------------------------------------------------------------
// writeHeaderIfNewFile()
// -----------------------------------------------------------------------------
bool SDManager::writeHeaderIfNewFile(const String &filename) {
    if (_sd.exists(filename.c_str())) {
        return true; // Ya existe, no se reescribe el encabezado.
    }

    FsFile file = _sd.open(filename.c_str(), O_WRITE | O_CREAT);
    if (!file) {
        Serial.println(F("[SDManager] ERROR: no se pudo crear el archivo CSV."));
        return false;
    }

    file.println(F(
        "nodeId,sequenceNumber,tempCanopyTopC,tempCanopyMidC,tempCanopyLowC,"
        "tempBaseC,humBasePct,pressureHpa,soilMoisturePct,batteryMilliVolts,statusFlags"
    ));
    file.close();
    return true;
}

// -----------------------------------------------------------------------------
// logPayload()
// -----------------------------------------------------------------------------
bool SDManager::logPayload(const TelemetryPayload &payload) {
    if (!_ready) {
        Serial.println(F("[SDManager] ERROR: SD no montada, se descarta el registro."));
        return false;
    }

    const String filename = buildFilenameForToday();

    if (!writeHeaderIfNewFile(filename)) {
        return false;
    }

    FsFile file = _sd.open(filename.c_str(), O_WRITE | O_APPEND);
    if (!file) {
        Serial.println(F("[SDManager] ERROR: no se pudo abrir el CSV para escritura."));
        return false;
    }

    // Desempaquetado a unidades físicas reales usando los MISMOS factores
    // de escala que usó el Emisor para empaquetar (telemetry.h): así el
    // CSV queda en °C/%RH/hPa/g/°/s, no en enteros crudos difíciles de leer.
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

    file.close(); // Incluye flush: los datos quedan persistidos antes de continuar.
    return true;
}

#endif // NODE_ROLE_RECEPTOR
