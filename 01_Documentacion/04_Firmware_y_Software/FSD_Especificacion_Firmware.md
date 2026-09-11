### 1. Descripción General de la Arquitectura

El proyecto adopta un enfoque de **monorepositorio modular** que administra dos binarios de firmware totalmente independientes compilados mediante _build flags_ condicionales (`-D NODE_ROLE_EMISOR` y `-D NODE_ROLE_RECEPTOR`):

1. **Nodo Emisor (Campo):** Orientado a ultra-bajo consumo energético, gobernado por una FSM lineal no bloqueante ejecutada tras despertar de _Deep Sleep_ cada 15 minutos.
    
2. **Nodo Receptor (Gateway Galpón):** Operación fija 24/7 en escucha continua LoRa P2P, con almacenamiento redundante en tarjeta MicroSD (librería `SdFat`, con CS en **D4/GPIO5**) y puente de retransmisión por tiempo compartido (_Time-Multiplexed_) hacia **The Things Network (TTN / LoRaWAN AU915 Sub-banda 2)**.
    

### 2. Nodo Emisor (Campo): FSM Lineal y Ciclo de Vida

El nodo emisor permanece en modo **Deep Sleep** (~0.156 mA) el **99.4% del tiempo**. En cada despertar, no ejecuta un bucle continuo `loop()`, sino una secuencia lineal dentro de `setup()` que incluye operaciones síncronas con timeouts acotados (ej. $100\text{ ms}$ max por transacción I2C) y culmina reiniciando el dispositivo al entrar a suspensión:

Plaintext

```
 ┌────────────────────────────────────────────────────────┐
 │                    STATE_DEEP_SLEEP                    │
 │         (XIAO ESP32-S3 en reposo @ ~0.156 mA)          │
 └───────────────────────────┬────────────────────────────┘
                             │ Timer RTC Alarm (15 Minutos / 900 s)
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                     STATE_INIT                         │
 │  - Inicializar Serial (Debug CDC)                      │
 │  - Auto-recuperación I2C (recoverBus 9 pulsos SCL)    │
 └───────────────────────────┬────────────────────────────┘
                             │ Init Completado
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                    STATE_READ_SENSORS                  │
 │  - PCA9548A Ch 0, 1, 2 @ 100 kHz: 3x MLX90614 (Copas)  │
 │  - PCA9548A Ch 3 @ 400 kHz: BME280 (Base)              │
 │  - ADC D6: Humedad Suelo / ADC D7: Divisor Batería     │
 └───────────────────────────┬────────────────────────────┘
                             │ Lecturas Completadas
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                   STATE_EVAL_ALERTS                    │
 │  - Evaluar Temp Foliar <= 2.0 °C  -> FLAG_ALERT_FROST  │
 │  - Evaluar Temp Foliar > 15.0 °C  -> FLAG_ALERT_HEAT   │
 └───────────────────────────┬────────────────────────────┘
                             │ Flags de Estado Compilados
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                   STATE_SAVE_LITTLEFS                  │
 │  - Montar sistema de archivos LittleFS (Flash 8 MB)    │
 │  - Serializar struct TelemetryPayload (23 Bytes)       │
 │  - Escribir en /telemetria_local.csv y file.close()    │
 └───────────────────────────┬────────────────────────────┘
                             │ Persistencia Local OK
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                     STATE_LORA_TX                      │
 │  - Configurar SX1262 (915 MHz, BW 125kHz, SF7, +22dBm) │
 │  - Transmitir ráfaga P2P binaria (23 Bytes)            │
 └───────────────────────────┬────────────────────────────┘
                             │ TX Concluida / Timeout
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                     STATE_SLEEP_PREP                   │
 │  - Calcular próximo intervalo + Jitter aleatorio (±5s) │
 │  - Actualizar RTC_DATA_ATTR (seq_number, error_count)  │
 │  - Desconectar periféricos / esp_deep_sleep_start()    │
 └───────────────────────────┴────────────────────────────┘
```

#### Persistencia en Memoria SRAM RTC (`RTC_DATA_ATTR`)

Para sobrevivir a los reinicios fríos de _Deep Sleep_, los siguientes contadores se preservan:

- `RTC_DATA_ATTR uint32_t g_sequenceNumber;` -> Contador incremental de tramas transmitidas.
    
- `RTC_DATA_ATTR uint16_t rtc_i2c_error_count;` -> Contador acumulado de eventos de recuperación del bus I2C.
    

### 3. Especificación de Módulos del Firmware

#### A. Gestor de Bus I2C y Multiplexión (`i2c_bus` / `pca9548a`)

- **Auto-recuperación de Bus (`I2CBus::recoverBus`):** Si una línea SDA se bloquea por ruido en los cables extendidos Cat6, el ESP32 genera hasta 9 pulsos manuales de reloj en SCL (D5) para liberar el esclavo antes de iniciar `Wire`.
    
- **Conmutación Dinámica de Frecuencia (`PCA9548A::selectChannel`):**
    
    - **Canales 0, 1 y 2 (MLX90614 Copas Top/Mid/Low):** Conmutan a **100 kHz** (`I2C_FREQ_EXTENDED_HZ`) para garantizar la integridad de la señal sobre extensores P82B715 de hasta 10 m.
        
    - **Canal 3 (BME280 Base):** Opera a **400 kHz** (`I2C_FREQ_LOCAL_HZ`).
        

#### B. Módulo de Lectura de Sensores (`sensor_manager`)

- **Procesamiento Analógico Calibrado:**
    
    - **Batería (ADC D7):** Utiliza `analogReadMilliVolts()` multiplicando por la relación del divisor resistivo $1:1$ (`BATTERY_DIVIDER_RATIO = 2.0f`).
        
    - **Suelo Capacitivo (ADC D6):** Aplica escalado lineal con `constrain()` basado en los umbrales `SOIL_ADC_DRY_RAW` y `SOIL_ADC_WET_RAW`.
        
- **Evaluación de Alertas Agronómicas:** Evalúa la referencia térmica foliar (mínimo entre MLX Top, Mid y Low; BME Base como respaldo). Dispara bitmask en `statusFlags`:
    
    - `FLAG_ALERT_FROST` (`0x0100`): Disparado si $T \le 2.0\,^\circ\text{C}$.
        
    - `FLAG_ALERT_HEAT_WINTER` (`0x0200`): Disparado si $T > 15.0\,^\circ\text{C}$.
        

#### C. Almacenamiento Local en Emisor (`littlefs_manager`)

Se ejecuta en `STATE_SAVE_LITTLEFS` dentro de `setup()`. Monta la partición en la memoria Flash SPI de 8 MB del ESP32-S3. Escribe registros en `/telemetria_local.csv` asegurando la persisencia con `file.close()` inmediato antes de apagar el chip.

### 4. Firmware del Gateway Receptor (Galpón)

El nodo receptor opera sin _Deep Sleep_ impulsado por la máquina de estados en `main.cpp`:

1. **Estado `LISTEN_P2P`:** Escucha continua en radiofrecuencia LoRa P2P (915 MHz).
    
2. **Recepción e Integridad:** Al recibir un paquete por la interrupción en **D0 (GPIO1 / DIO1)**, el método `receiveP2P()` de `lora_manager.cpp` valida estrictamente que la longitud devuelta por la radio sea exactamente de **23 Bytes** (`radio.getPacketLength() == sizeof(TelemetryPayload)`).
    
3. **Persistencia Masiva en MicroSD (`sd_manager`):** Guarda de inmediato el 100% de las lecturas válidas en la tarjeta MicroSD (bus SPI compartida con CS en **D4/GPIO5**) en la ruta `/telemetria_acumulado.csv` mediante la librería **SdFat**.
    
4. **Estado `BRIDGE_TTN` (Time-Multiplexing cada 5 min):**
    
    - Transcurridos `TTN_UPLINK_INTERVAL_MS` (5 minutos), el receptor conmuta su módem SX1262 a la pila **LoRaWAN (AU915 Sub-banda 2)** empleando el objeto persistente `LoRaWANNode* _ttnNode`.
        
    - Reenvía únicamente el **último payload de telemetría recibido**. Nota: El cumplimiento del límite de 30 s/día de _airtime_ de la Fair Use Policy de TTN depende de la combinación de tamaño de trama (23 bytes), Spreading Factor y frecuencia de transmisión de uplink.
        
    - Re-establece la escucha continua P2P de inmediato mediante `beginP2PListen()`.
        

### 5. Configuración de Entornos de Compilación (`platformio.ini`)

Ini, TOML

```
[platformio]
default_envs = nodo_emisor

[env]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
upload_speed = 921600
monitor_speed = 115200
build_flags = 
	-D ARDUINO_USB_CDC_ON_BOOT=1
	-D ARDUINO_USB_MODE=1
	-std=gnu++17
lib_deps = 
	jgromes/RadioLib @ ^6.6.0
	adafruit/Adafruit BME280 Library @ ^2.2.4
	adafruit/Adafruit MLX90614 Library @ ^2.1.5
	adafruit/Adafruit Unified Sensor @ ^1.1.14
	greiman/SdFat @ ^2.2.3

[env:nodo_emisor]
build_flags = 
	${env.build_flags}
	-D NODE_ROLE_EMISOR=1

[env:nodo_receptor]
build_flags = 
	${env.build_flags}
	-D NODE_ROLE_RECEPTOR=1
```

### Verificación de Compilación Cruzada con PlatformIO

Para certificar que los 10 puntos de sincronización han quedado cerrados en el código y la especificación técnica, se debe validar la compilación limpia ejecutando en PowerShell:

PowerShell

```
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e nodo_emisor
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e nodo_receptor
```