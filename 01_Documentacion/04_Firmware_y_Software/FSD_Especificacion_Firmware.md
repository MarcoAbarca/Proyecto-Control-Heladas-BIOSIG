## 1. Descripción General
PlatformIO + Arduino (C++), Seeed XIAO ESP32-S3. Un solo `platformio.ini` con **dos entornos** (`nodo_emisor`/`nodo_receptor`, build flags `NODE_ROLE_EMISOR`/`NODE_ROLE_RECEPTOR`), no dos repos separados. Compilar con `pio run -e nodo_emisor` o `-e nodo_receptor`.

## 2. FSM del Nodo Emisor (lineal, no loop continuo)
Cada Deep Sleep reinicia el chip: `setup()` corre una vez por ciclo y termina en `esp_deep_sleep_start()` (nunca retorna). No hay `switch(state)` en loop() — es una secuencia de funciones.

```
Wakeup (timer RTC 15min) → I2C&Sensor Init → Read Sensors → Evaluate Alerts
  → Save LittleFS → LoRa P2P TX → enterDeepSleep()
```

* **I2C&Sensor Init:** `I2CBus::begin()`; si el bus está atascado, `recoverBus()`. Si `mux.begin()` falla, se activa `FLAG_I2C_BUS_ERROR` y se **saltan directo** las 4 lecturas I2C (optimización: evita ~400ms de timeouts) yendo directo a `readSoilAndBatteryOnly()`.
* **Read Sensors:** `SensorManager::readAllInto()` — 3x MLX90614 (Ch0-2, 100kHz) + BME280 (Ch3, 400kHz) + suelo/batería por ADC (no dependen del mux).
* **Evaluate Alerts:** `evaluateAlerts()` en main.cpp, compara contra `FROST_THRESHOLD_C`/`OVERHEAT_THRESHOLD_C`.
* **Save LittleFS:** `LittleFSManager::logPayload()`, CSV en `/telemetria_local.csv` (texto, no binario — corrige versión anterior que hablaba de `log.bin`).
* **LoRa P2P TX:** se transmite SIEMPRE, incluso con payload degradado (bus caído) — es la "señal de vida" del nodo, no hay un modo de emergencia separado.
* **RTC_DATA_ATTR:** `g_sequenceNumber` (persiste el contador) y `g_consecutiveEmptyReads` (diagnóstico, no viaja en el payload — los 16 bits de statusFlags ya están asignados).

## 3. Módulos
### SensorManager (Ch0-2 MLX90614, Ch3 BME280)
Pines I2C D4(SDA)/D5(SCL). Aislamiento estricto: `PCA9548A::selectChannel(canal, frecuencia)` nunca activa más de un canal a la vez (los 3 MLX90614 comparten dirección 0x5A).

### LittleFSManager (solo Emisor)
`LittleFS.begin(true)` (auto-formatea si falla el montaje). Log en `/telemetria_local.csv`, texto CSV con header + append, `file.close()` (flush) antes de dormir.

### LoRaManager (RadioLib, SX1262)
- Emisor: `transmitPayload()`, bloqueante pero acotado (~100-200ms a SF7).
- Receptor: time-multiplexa el mismo radio entre escucha P2P continua y bridging LoRaWAN/TTN periódico (`TTN_UPLINK_INTERVAL_MS`, 5min por defecto) — no puede hacer ambas cosas a la vez con un solo módulo de radio.
- Nota abierta: `bridgeToTTN()` recrea el objeto `LoRaWANNode` en cada ciclo; pendiente evaluar si eso rompe la continuidad del contador de tramas de TTN (ver más abajo).

## 4. Firmware del Receptor
Loop continuo (sin Deep Sleep, alimentación por cable):
1. `LISTEN_P2P`: `checkReceivedP2P()` no bloqueante. Cada paquete recibido se guarda COMPLETO en MicroSD vía `SDManager` (`SdFat`, no `SD.h`/`SD.begin()` como decía la versión anterior).
2. Cada `TTN_UPLINK_INTERVAL_MS`: `BRIDGE_TTN` — sube a TTN solo el ÚLTIMO payload pendiente (no cada paquete P2P; LoRaWAN no aguanta ese volumen), y vuelve a modo P2P.
3. No hay salida JSON por Serial (la versión anterior lo mencionaba; no está implementado).

## 5. platformio.ini (real, ver 05_Code/platformio.ini)
```ini
[platformio]
default_envs = nodo_emisor
[env]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
upload_speed = 921600
monitor_speed = 115200
build_flags = -D CORE_DEBUG_LEVEL=3 -D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    adafruit/Adafruit BME280 Library @ ^2.2.4
    adafruit/Adafruit Unified Sensor @ ^1.1.14
    adafruit/Adafruit MLX90614 Library @ ^2.1.5
    jgromes/RadioLib @ ^6.6.0
monitor_filters = esp32_exception_decoder, time

[env:nodo_emisor]
build_flags = ${env.build_flags} -D NODE_ROLE_EMISOR

[env:nodo_receptor]
build_flags = ${env.build_flags} -D NODE_ROLE_RECEPTOR
lib_deps = ${env.lib_deps}
    greiman/SdFat @ ^2.2.3
```
*(La versión anterior mostraba un solo entorno sin roles y librerías descartadas hace varias iteraciones: Adafruit MPU6050, wollewald/SHT3xDIS. No se usan.)*

## 6. Pendientes conocidos (no ocultar)
* TTN/LoRaWAN: en pausa operativa hasta confirmación explícita (ver Hoja de Ruta) — el código existe pero no se ha validado en campo contra un DevEUI real.
* Persistencia de sesión LoRaWAN en el Receptor entre ciclos de bridging.
* Downlink para umbrales dinámicos: no implementado, `FROST_THRESHOLD_C`/`OVERHEAT_THRESHOLD_C` son fijos en config.h.
* Calibración de la sonda de suelo (`SOIL_ADC_DRY_RAW`/`SOIL_ADC_WET_RAW`): placeholders sin calibrar.
