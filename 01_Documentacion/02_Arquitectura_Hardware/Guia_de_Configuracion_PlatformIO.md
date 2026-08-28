## 1. Propósito y Separación de Entornos

Para mantener un flujo de trabajo estructurado y libre de errores de integración:

- **Arduino IDE:** Reservado exclusivamente para pruebas rápidas aisladas en mesa (banco de pruebas) de componentes individuales (comprobación de dirección I2C, prueba de ráfaga LoRa simple).
- **PlatformIO (VS Code):** Entorno oficial de desarrollo e integración del proyecto. Maneja la compilación modular, control de dependencias estrictas, estructura de librerías y generación de binarios para el Seeed Studio XIAO ESP32-S3.

## 2. Requisitos Previos e Instalación

1. **Visual Studio Code:** Tener instalada la versión más reciente de VS Code.
2. **Extensión PlatformIO IDE:** Buscar e instalar la extensión oficial `PlatformIO IDE` en el Marketplace de VS Code.
3. **Drivers de Puerto Serie (USB CDC):** Asegurar la detección del chip ESP32-S3 a través del puerto USB nativo.

## 3. Estructura del Archivo de Configuración (`platformio.ini`)

A continuación se especifica la configuración exacta del archivo `platformio.ini` para el **Nodo Emisor de Campo**. Este archivo gestiona la plataforma, el marco de trabajo (_framework_), la asignación de memoria Flash y las librerías requeridas.

Ini, TOML

```
; =====================================================================
; PlatformIO Configuration File - Control Heladas (BIOSIG)
; Proyecto: Nodo Emisor de Campo (XIAO ESP32-S3)
; =====================================================================

[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino

; Serial Monitor y Carga de Firmware
monitor_speed = 115200
upload_speed = 921600
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L

; Configuración de Memoria y Sistema de Archivos
board_build.filesystem = littlefs
board_build.partitions = default_8MB.csv

; Banderas de Compilación para USB Nativo y Depuración
build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CONFIG_LITTLEFS_FOR_IDF_3_2

; Dependencias de Librerías (Gestión Automática)
lib_deps = 
    jgromes/RadioLib @ ^6.6.0                      ; Control avanzado módulo LoRa SX1262
    adafruit/Adafruit BME280 Library @ ^2.2.4      ; Sensor de microclima base
    adafruit/Adafruit MLX90614 Library @ ^2.1.5    ; Sensores infrarrojos de canopia
    adafruit/Adafruit BusIO @ ^1.16.1               ; Soporte unificado de bus I2C/SPI
```

## 4. Estructura de Directorios del Código Fuente (`03_Firmware/`)

Dentro de la raíz de tu repositorio, el firmware del proyecto en PlatformIO debe organizarse de acuerdo al siguiente esquema modular:

Plaintext

```
03_Firmware/
└── Nodo_Emisor_Campo/
    ├── .pio/                             <-- Archivos temporales de compilación
    ├── include/                          <-- Archivos de cabecera (.h)
    │   ├── config.h                      <-- Asignación de pines y constantes I2C
    │   ├── lora_protocol.h               <-- Definición de estructura del Payload
    │   └── sensors_manager.h             <-- Controladores de BME280 y MLX90614
    ├── lib/                              <-- Librerías locales personalizadas
    ├── src/                              <-- Código fuente principal (.cpp)
    │   ├── main.cpp                      <-- Máquina de estados y Deep Sleep
    │   ├── sensors_manager.cpp           <-- Manejo de multiplexor TCA9548A
    │   └── lora_protocol.cpp             <-- Ráfaga de transmisión RadioLib
    └── platformio.ini                    <-- Archivo de configuración mostrado arriba
```

## 5. Procedimiento de Compilación y Flasheo

1. **Compilar Firmware:** Presionar el botón `Build` (icono de marca de verificación `✓` en la barra inferior de PlatformIO) o ejecutar en terminal:
    
    Bash
    
    ```
    pio run
    ```
    
2. **Cargar al Microcontrolador:** Conectar el XIAO ESP32-S3 por USB-C y presionar `Upload` (icono de flecha `→`) o ejecutar:
    
    Bash
    
    ```
    pio run --target upload
    ```
    
3. **Monitorear Salida Serie:** Abrir la consola serie para ver logs de depuración:
    
    Bash
    
    ```
    pio device monitor
    ```
    

## 6. Manejo de Errores Comunes

- **Error de Puerto COM / USB no detectado:** El XIAO ESP32-S3 utiliza puerto USB CDC nativo. Si la placa entra en bucle o no responde, mantener presionado el botón **BOOT**, presionar y soltar el botón **RESET**, y luego soltar **BOOT** para forzar el modo de descarga (_ROM Bootloader_).
- **Fallo de montaje de LittleFS:** Si la memoria Flash interna falla al inicializar, ejecutar la orden de formateo de partición en el código usando `LittleFS.begin(true)` para forzar la creación del sistema de archivos en la primera ejecución.