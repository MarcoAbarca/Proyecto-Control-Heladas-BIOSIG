Sistema IoT de monitoreo térmico de gradiente foliar y suelo para la gestión y alerta temprana de heladas agrotécnicas en cultivos agrícolas.

##  Resumen del Proyecto

El sistema recopila datos de temperatura ambiental y foliar en tres estratos (superior, medio e inferior) de la canopia del árbol mediante sensores de radiación infrarroja, sumado a la medición de temperatura y humedad en suelo. La información es procesada localmente por un nodo emisor desplegado en campo y transmitida vía radioenlace LoRa (915 MHz) hacia una estación receptora (Gateway) situada en galpón/oficina para su procesamiento y almacenamiento.

## 🏗️ Arquitectura del Sistema
[ CAMPO / ÁRBOL ]                                       [ GALPÓN / OFICINA ]

 Panel Solar (5V/1W)                                    Fuente USB 5V/2A
       │                                                       │
 2x Baterías 18650                                             ▼
 (6800 mAh Li-Ion)                                   ┌───────────────────┐
       │                                             │ XIAO ESP32-S3 +   │
       ▼                                             │ SX1262 (Gateway)  │
┌───────────────┐          LoRa 915 MHz              └─────────┬─────────┘
│ XIAO ESP32-S3 ├──────────────────────────────────────────────┘
│ + Wio-SX1262  │ (Ráfaga cada 15 min / Deep Sleep)            │ SPI
└───────┬───────┘                                              ▼
        │                                            ┌───────────────────┐
        ├─ Flash Interna (LittleFS 8 MB)             │ Lector MicroSD    │
        │                                            │ (DATALOG.CSV)     │
        ├─ PCA9548A (Multiplexor I2C)                └───────────────────┘
        │   ├─ Ch 0-2: Extensor P82B715 -> MLX90614 (Top/Mid/Low)
        │   ├─ Ch 3: BME280 (base, local)
        │
        └─ Sensor Capacitivo / Temperatura de Suelo

## 💻 Unidades del Sistema y Alimentación

### 1. Nodo Emisor (Campo)

- **Microcontrolador:** Seeed Studio XIAO ESP32-S3.
    
- **Módulo LoRa:** Wio-SX1262 (915 MHz).
    
- **Alimentación:** Banco de 2 baterías Li-Ion 18650 (6800 mAh en paralelo) + Cargador TP4056 + Diodo Schottky + 1 Panel Solar de 5V / 1W.
    
- **Estrategia Energética:** Operación bajo ciclo de trabajo intermitente con modo _Deep Sleep_ el 99.4% del tiempo.
    
- **Consumo Estimado:** ~0.156 mA en reposo profundo / ~200 mA en ráfaga activa. Consumo diario aproximado: **30.48 mAh/día**.
    
- **Respaldo Local:** Memoria Flash de 8 MB integrada en la placa XIAO ESP32-S3 (sistema de archivos LittleFS) para prevenir conflictos en el bus SPI dedicado al módulo LoRa.
    

### 2. Nodo Receptor / Gateway (Galpón)

- **Microcontrolador:** Seeed Studio XIAO ESP32-S3.
    
- **Módulo LoRa:** Wio-SX1262 (915 MHz) en modo de escucha continua (_Continuous Rx_).
    
- **Alimentación:** Adaptador de pared USB 5V / 2A (alimentación eléctrica continua 24/7).
    
- **Consumo Estimado:** ~96 mA continuos (0.48 W).
    
- **Respaldo Local:** Módulo Lector MicroSD por bus SPI + Tarjeta MicroSD 16GB (FAT32) para almacenamiento continuo de tramas en formato CSV.
    

## 📂 Estructura del Repositorio
Proyecto-Control-Heladas-Biosig/
├── .gitignore
├── README.md
├── 01_Documentacion/
│   ├── Indice.md
│   ├── 01_Especificaciones/
│   ├── 02_Arquitectura_Hardware/
│   ├── 03_Protocolos_Datos/
│   ├── 04_Firmware_y_Software/
│   └── 05_Gestion_y_Cotizaciones/
├── 02_Diseno_Hardware/
│   ├── Esquematicos/
│   └── Modelos_3D/
├── 03_Firmware/
│   └── Test/
├── 04_Panel_Nube/
│   └── payload_formatter.js
└── 05_Code/
    ├── include/       (Configuración y encabezados C++)
    ├── src/           (Firmware C++: nodo_emisor y nodo_receptor)
    ├── backend/       (API Node.js + Ingesta TTN + Dashboard Web)
    └── platformio.ini (Entornos de compilación PlatformIO)
## 🛠️ Tecnologías y Entorno de Desarrollo

- **Entorno de Desarrollo:** PlatformIO / Framework Arduino (C++).
    
- **Gestor de Notas / Documentación Local:** Obsidian (Bóveda sincronizada en `01_Documentacion/`).
    
- **Librerías Principales:** `RadioLib` (SX1262), `Adafruit_MLX90614`, `Wire` (I2C non-blocking), `LittleFS`.