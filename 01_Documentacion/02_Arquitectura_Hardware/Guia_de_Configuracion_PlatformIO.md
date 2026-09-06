# Guía de Configuración del Entorno de Desarrollo (PlatformIO)

## 1. Requisitos
* VS Code + Extensión PlatformIO IDE.
* Board Target: Seeed Studio XIAO ESP32-S3.

## 2. platformio.ini real (05_Code/platformio.ini)
**Dos entornos, no uno.** Corrige versión anterior que mostraba `[env:seeed_xiao_esp32s3]` único sin roles ni RadioLib/MLX90614.
```ini
[platformio]
default_envs = nodo_emisor
[env]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
build_flags = -D CORE_DEBUG_LEVEL=3 -D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    adafruit/Adafruit BME280 Library @ ^2.2.4
    adafruit/Adafruit Unified Sensor @ ^1.1.14
    adafruit/Adafruit MLX90614 Library @ ^2.1.5
    jgromes/RadioLib @ ^6.6.0
[env:nodo_emisor]
build_flags = ${env.build_flags} -D NODE_ROLE_EMISOR
[env:nodo_receptor]
build_flags = ${env.build_flags} -D NODE_ROLE_RECEPTOR
lib_deps = ${env.lib_deps}
    greiman/SdFat @ ^2.2.3
```
Compilar: `pio run -e nodo_emisor` o `pio run -e nodo_receptor`.

## 3. Estructura real de 05_Code
```
05_Code/
├── include/ (config.h, i2c_bus.h, pca9548a.h, sensor_manager.h, telemetry.h, lora_manager.h, sd_manager.h, littlefs_manager.h)
├── src/ (main.cpp, i2c_bus.cpp, pca9548a.cpp, sensor_manager.cpp, lora_manager.cpp, sd_manager.cpp, littlefs_manager.cpp)
└── platformio.ini
```
*(Corrige versión anterior: no existe `lora_protocol.cpp`, el archivo real es `lora_manager.cpp`; faltaban sd_manager y littlefs_manager en el listado.)*

## 4. Compilar / Flashear / Monitorear
```
pio run -e nodo_emisor
pio run -e nodo_emisor --target upload
pio device monitor
```

## 5. Troubleshooting
* **USB no detectado / bootloop:** mantener BOOT, presionar y soltar RESET, soltar BOOT (modo ROM Bootloader).
* **LittleFS no monta:** `LittleFS.begin(true)` ya fuerza auto-formateo en el firmware actual (littlefs_manager.cpp), no requiere intervención manual.
