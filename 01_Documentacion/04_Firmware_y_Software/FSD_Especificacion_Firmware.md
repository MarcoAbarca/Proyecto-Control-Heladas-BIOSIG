## 1. Descripción General

El firmware está desarrollado sobre el entorno **PlatformIO** utilizando el framework **Arduino (C++)** para el microcontrolador **Seeed Studio XIAO ESP32-S3**. El proyecto utiliza un enfoque de compilación condicional dentro de un único monorepositorio para gestionar dos binarios independientes:

1. **Nodo Emisor (Campo):** Arquitectura orientada a ultra-bajo consumo impulsada por eventos y temporización RTC.
2. **Nodo Receptor (Gateway Galpón):** Arquitectura de escucha activa continua, procesamiento de paquetes LoRa, almacenamiento en MicroSD y envío de telemetría hacia servicios en la nube.

## 2. Máquina de Estados del Nodo Emisor (Campo)

El firmware del nodo emisor opera como una máquina de estados finitos (FSM) que permanece en modo **Deep Sleep** el $99.4\%$ del tiempo para garantizar la autonomía energética a partir del banco de baterías 18650 y la celda solar.

Plaintext

```
 ┌────────────────────────────────────────────────────────┐
 │                    STATE_DEEP_SLEEP                    │
 │         (XIAO ESP32-S3 en reposo @ ~0.156 mA)          │
 └───────────────────────────┬────────────────────────────┘
                             │ Timer RTC Alarm (15 Minutos)
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                     STATE_INIT                         │
 │  - Inicializar Power Rails & Bus I2C / SPI             │
 │  - Montar sistema de archivos LittleFS                 │
 └───────────────────────────┬────────────────────────────┘
                             │ Init OK
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                    STATE_READ_SENSORS                  │
 │  - Conmutar Mux TCA9548A (Canales 1, 2, 3 para IRs)    │
 │  - Leer BME280 (Canal 0) y Sonda Capacitiva Suelo      │
 └───────────────────────────┬────────────────────────────┘
                             │ Lecturas completadas
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                   STATE_SAVE_LOCAL                     │
 │  - Empaquetar struct LoRaPayload                       │
 │  - Escribir respaldo en Flash Interna (LittleFS)       │
 └───────────────────────────┬────────────────────────────┘
                             │ Persistencia OK
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                     STATE_LORA_TX                      │
 │  - Inicializar SX1262 (Frecuencia: 915 MHz, SF7)       │
 │  - Transmitir ráfaga binaria (15 Bytes, +22 dBm)       │
 └───────────────────────────┬────────────────────────────┘
                             │ TX Done / Timeout (5s max)
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                     STATE_SLEEP_PREP                   │
 │  - Desconectar buses I2C / SPI                         │
 │  - Configurar RTC Timer wakeup (900 segundos)          │
 │  - Ejecutar esp_deep_sleep_start()                     │
 └────────────────────────────────────────────────────────┘
```

## 3. Especificación Módulos del Firmware

### A. Módulo I2C y Multiplexión (`SensorManager`)

- **Gestión de Buses:** El bus I2C nativo opera en los pines `D4` (SDA) y `D5` (SCL) del XIAO ESP32-S3 a una frecuencia de $100\text{ kHz}$ (Standard Mode) para asegurar la integridad de la señal a través de los extensores de bus P82B715 y el cable Cat6.
    
- **Rutina del Multiplexor TCA9548A:**
    
    1. Escribir registro `0x70` con mascara de bit `0x02` (Canal 1 - Sensor Foliar Superior).
    2. Adquirir temperatura de objeto y ambiente desde el sensor MLX90614.
    3. Replicar para Canal 2 (`0x04` - Foliar Medio) y Canal 3 (`0x08` - Foliar Inferior).
    4. Desactivar todos los canales escribiendo `0x00` en el Mux para evitar colisiones en la línea.

### B. Módulo de Almacenamiento Local (`StorageManager`)

- **Sistema de Archivos:** `LittleFS` configurado sobre la memoria Flash SPI interna de 8 MB del XIAO ESP32-S3.
    
- **Prevención de Corrupción:** El archivo de registro `log.bin` se abre exclusivamente en modo apéndice (`"ab"`) durante el ciclo activo y se ejecuta un `file.flush()` y `file.close()` inmediato antes de entrar en suspensión.
    

### C. Módulo de Transmisión LoRa (`RadioManager`)

- **Librería Base:** `RadioLib` configurada para el chip Semtech SX1262 acoplado al módulo Wio-SX1262.
- **Control de Errores y Timeouts:**
    - Si la inicialización del módem falla o la transmisión no completa la interrupción `DIO1` en menos de 2 segundos, el firmware incrementa el registro de errores en la RTC RAM y fuerza el ingreso a _Deep Sleep_ para evitar el agotamiento de la batería por bloqueos de software.

## 4. Firmware del Gateway Receptor (Oficina)

A diferencia del nodo emisor, el Gateway se ejecuta en un bucle continuo de procesamiento sin modo de ahorro de energía:

1. **Escucha LoRa Activa (`RX_CONTINUOUS`):** Mantiene el módem SX1262 en modo de recepción continua a 915 MHz.
2. **Interrupción por Paquete Entrante:** Al activarse el pin `DIO1` (Packet Received):
    - Extrae la trama binaria de 15 Bytes.
        - Verifica la integridad de la longitud y estructura contra la firma `sizeof(LoRaPayload)`.
3. **Persistencia en MicroSD:** Desempaqueta los campos de datos y escribe una nueva fila en el archivo `DATALOG.CSV` montado sobre la tarjeta MicroSD externa vía el bus SPI dedicado.

4. **Retransmisión Serie / Cloud:** Formatea el paquete desempaquetado a una cadena en formato JSON estandarizado y la emite a través de la interfaz UART USB (`Serial`) para consumo de dashboards locales o aplicaciones de telemetría.    
## 5. Configuración de PlatformIO (`platformio.ini`)
Ini, TOML

```
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
upload_speed = 921600
monitor_speed = 115200
build_flags = 
    -D CORE_DEBUG_LEVEL=3
    -D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps = 
    adafruit/Adafruit BME280 Library @ ^2.2.4
    adafruit/Adafruit Unified Sensor @ ^1.1.14
    adafruit/Adafruit MPU6050 @ ^2.2.6
    wollewald/SHT3xDIS @ ^1.0.2
monitor_filters = 
    esp32_exception_decoder
    time
```