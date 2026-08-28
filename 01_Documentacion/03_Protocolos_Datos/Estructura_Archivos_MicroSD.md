## 1. Visión General de la Persistencia

El sistema implementa dos niveles de almacenamiento offline para garantizar que no se pierda ningún dato de telemetría ante fallas de red, cortes de energía o caídas del enlace LoRa:

1. **Memoria Flash SPI Interna (Nodo Emisor de Campo):** Utiliza el sistema de archivos **LittleFS** en la Flash integrada de 8 MB del Seeed Studio XIAO ESP32-S3. Sirve como búfer circular local para almacenar las mediciones en caso de que la transmisión LoRa falle o no reciba confirmación.
    
2. **Tarjeta MicroSD (Nodo Gateway / Receptor):** Utiliza un sistema de archivos **FAT32** estándar sobre el bus SPI dedicado del Gateway. Almacena de forma permanente todas las tramas recibidas desde el campo en archivos con formato de valores separados por comas (`.CSV`).
    

## 2. Estructura de Directorios en la Tarjeta MicroSD (Gateway)

Para evitar el degradado de rendimiento en tarjetas MicroSD provocado por la acumulación de miles de archivos en una sola carpeta raíz, la tarjeta organiza los datos jerárquicamente por **Año** y **Mes**:

Plaintext

```
/ (Raíz MicroSD FAT32)
├── SYSTEM.LOG                      <-- Registro general de eventos y arranque del Gateway
└── LOGS/
    └── YYYY/                       <-- Carpetas por Año (ej: 2026)
        └── MM/                     <-- Carpetas por Mes (ej: 08 para Agosto)
            ├── DATALOG_YYYYMMDD.CSV <-- Archivo diario de datos de telemetría
            └── DATALOG_20260828.CSV
```

### Reglas de Nombramiento de Archivos

- **Archivos Diarios:** `DATALOG_YYYYMMDD.CSV` (ejemplo: `DATALOG_20260828.CSV`).
    
- **Rotación Automatizada:** A las `00:00:00` horas (vía RTC/NTP), el Gateway cierra el archivo del día anterior y crea/abre el nuevo archivo correspondiente al nuevo día.
    

## 3. Formato y Encabezados del Archivo `.CSV` (Telemetría de Campo)

Cada línea en el archivo `DATALOG_YYYYMMDD.CSV` corresponde a una trama binaria LoRa de 15 bytes recibida y decodificada exitosamente por el Gateway.

### Encabezado del Archivo (Fila 1)

Fragmento de código

```
Timestamp,Node_ID,Msg_ID,T_Top_C,T_Mid_C,T_Low_C,T_Amb_C,HR_pct,P_hPa,V_Bat_V,Soil_Moist_pct,RSSI_dBm,SNR_dB
```

### Diccionario de Columnas y Tipos de Datos

|**Columna**|**Tipo de Dato**|**Unidad**|**Descripción**|**Ejemplo**|
|---|---|---|---|---|
|`Timestamp`|String ISO-8601|`YYYY-MM-DD HH:MM:SS`|Fecha y hora de recepción registrada por el Gateway|`2026-08-28 01:15:00`|
|`Node_ID`|Entero sin signo|Hex / Int|Identificador único del nodo emisor de origen|`1`|
|`Msg_ID`|Entero sin signo|Contador|Número secuencial de mensaje transmitido (0-255)|`42`|
|`T_Top_C`|Flotante|°C|Temp. foliar infrarroja - Estrato Canopia Superior|`-1.25`|
|`T_Mid_C`|Flotante|°C|Temp. foliar infrarroja - Estrato Canopia Medio|`-0.80`|
|`T_Low_C`|Flotante|°C|Temp. foliar infrarroja - Estrato Canopia Inferior|`0.10`|
|`T_Amb_C`|Flotante|°C|Temperatura ambiental del sensor BME280|`1.50`|
|`HR_pct`|Flotante|%|Humedad relativa ambiental (0% - 100%)|`92.4`|
|`P_hPa`|Flotante|hPa|Presión atmosférica local|`1013.2`|
|`V_Bat_V`|Flotante|V|Voltaje de batería Li-Ion del nodo de campo|`3.95`|
|`Soil_Moist_pct`|Flotante|%|Humedad volumétrica capacitiva de suelo|`35.8`|
|`RSSI_dBm`|Entero con signo|dBm|Indicador de fuerza de señal de radio LoRa|`-85`|
|`SNR_dB`|Flotante|dB|Relación señal/ruido de la recepción LoRa|`8.5`|

### Ejemplo de Fila de Datos

Fragmento de código

```
2026-08-28 01:15:00,1,42,-1.25,-0.80,0.10,1.50,92.4,1013.2,3.95,35.8,-85,8.5
```

## 4. Esquema de Respaldo Local en Flash Interna (LittleFS en Nodo Emisor)

En la memoria Flash del **Nodo Emisor**, los datos se almacenan en un archivo único de registro de respaldo llamado `/offline_data.bin` utilizando un formato binario continuo de 15 bytes por registro (idéntico a la estructura del Payload LoRa).

### Algoritmo de Gestión de Respaldo LittleFS

1. **Muestreo:** El nodo de campo lee los sensores y genera el bloque de 15 bytes.
    
2. **Intento de Envío:** Se transmite la trama por radio LoRa.
    
3. **Escritura Fallback:** Si no se confirma el envío o la radio falla, la trama de 15 bytes se escribe directamente al final de `/offline_data.bin`.
    
4. **Sincronización:** En la siguiente reconexión exitosa, el nodo envía en ráfaga las tramas pendientes guardadas en `/offline_data.bin` y vacía el archivo local.
    
5. **Capacidad de Almacenamiento:** Con una partición LittleFS de 2 MB asignada, el nodo puede almacenar hasta **139,810 registros locales**, equivalente a más de **3.9 años continuos de mediciones offline** (a razón de 1 lectura cada 15 minutos).
    

## 5. Estrategia de Manejo de Errores y Ficheros Corruptos

- **Apertura y Cierre Seguro (_Flush/Sync_):** En el Gateway, el archivo `.CSV` debe abrirse, actualizarse con la nueva línea, ejecutar `file.flush()` o `file.close()` inmediatamente tras cada escritura. Esto previene la pérdida de la tabla FAT si ocurre un corte de energía mientras se escribe.
    
- **Verificación de Presencia de SD:** Antes de cada intento de escritura en el Gateway, el firmware verifica la presencia física del medio mediante `SD.begin()`. Si la tarjeta no está presente o falla, el Gateway conmuta a volcar las lecturas exclusivamente por el puerto USB CDC / Serie.