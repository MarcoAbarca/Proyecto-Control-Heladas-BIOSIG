## 1. Visión General
Dos niveles de respaldo local, AMBOS EN FORMATO CSV DE TEXTO (no binario):
1. **LittleFS (Emisor):** `/telemetria_local.csv` en la flash interna de 8MB. Se escribe TODOS los ciclos (no es condicional a que falle la transmisión LoRa — corrige versión anterior que describía retry/reenvío; eso no está implementado).
2. **MicroSD (Receptor, vía SdFat):** `/telemetria_acumulado.csv`, un único archivo acumulativo (no hay RTC/NTP en el Receptor, así que **no hay rotación diaria por fecha** — corrige versión anterior que describía `DATALOG_YYYYMMDD.CSV` en carpetas `/LOGS/YYYY/MM/`; eso es aspiracional, no implementado).

## 2. Archivo real del Receptor
Un solo archivo plano en la raíz: `/telemetria_acumulado.csv`. Sin subcarpetas por año/mes.

## 3. Formato y Encabezado (idéntico en Receptor y Emisor, mismos campos)
```
nodeId,sequenceNumber,tempCanopyTopC,tempCanopyMidC,tempCanopyLowC,tempBaseC,humBasePct,pressureHpa,soilMoisturePct,batteryMilliVolts,statusFlags
```
*(Corrige versión anterior: no hay columna `Timestamp` — sin RTC/NTP no hay hora real que registrar. No hay `RSSI_dBm`/`SNR_dB` en el CSV — el Receptor sí lee RSSI/SNR del paquete pero solo los imprime por Serial, no los escribe a SD. `V_Bat_V` no existe como tal: el campo real es `batteryMilliVolts`, entero en mV, no volts en float.)*

### Ejemplo de fila real
```
1,42,-1.25,-0.80,0.10,1.50,92.40,1013.20,35.80,3950,0x0140
```

## 4. Respaldo LittleFS del Emisor
Archivo: `/telemetria_local.csv` (texto CSV, header + append — **no** es `/offline_data.bin` binario como decía la versión anterior). Mismos campos que la Sección 3. `file.close()` (flush) inmediato tras cada escritura, antes de Deep Sleep.

No existe lógica de "cola de reenvío": cada ciclo se guarda en LittleFS Y se intenta transmitir por LoRa, sin condicionar uno al resultado del otro. Si se necesita reenvío de pendientes tras una falla de radio, es un desarrollo futuro, no algo ya implementado.

## 5. Manejo de errores (real)
* `SDManager::begin()` (Receptor) usa `SdFat::begin(SD_CS_PIN, SD_SCK_MHZ(20))`, no `SD.begin()` de la librería `SD.h` estándar.
* Si la SD no monta, el Receptor sigue operando (recibe P2P, sube a TTN) sin respaldo local — no hay volcado alternativo por Serial.
* `file.close()` tras cada fila en ambos casos (Receptor y Emisor) para minimizar pérdida ante corte de energía.
