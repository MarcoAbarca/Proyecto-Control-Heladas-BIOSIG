# Specification Requirements Specification (SRS)

## Nombre del Sistema
Sistema IoT de Monitoreo Térmico de Gradiente Foliar y Suelo para la Gestión y Alerta Temprana de Heladas Agrotécnicas (Control Heladas - BIOSIG)

---

## 1. Visión General y Alcance del Sistema

Este documento especifica los requerimientos funcionales, de hardware, ambientales y de desempeño para el sistema personalizado de monitoreo de heladas. El sistema opera bajo una topología de dos nodos (Nodo Emisor de Campo y Nodo Receptor/Gateway) diseñada para medir el temperatura en múltiples estratos de la canopia, las condiciones microclimáticas ambientales y las métricas del suelo en huertos agrícolas.

El objetivo principal es detectar y reportar capas de inversión térmica y eventos de heladas radiativas en tiempo real, transmitiendo datos desde ubicaciones remotas en campo hacia una estación central de monitoreo a través de radiofrecuencia de largo alcance y bajo consumo.


## 2. Entorno Operativo y Límites Ambientales

* **Rango Operativo en Campo:** -10 °C a +45 °C en ambientes agrícolas a la intemperie.
* **Límites de Humedad Relativa:** 10% a 100% HR (condiciones con condensación).
* **Protección Contra Ingreso (Gabinete de Control Principal):** Certificación IP65 para resistencia a la intemperie.
* **Protección Contra Ingreso (Sondas Externas y Cables):** Clasificación IP68 para prensacables (PG7/PG9) y encapsulados de sensores montados en ramas.
* **Exposición a Intemperie y Rayos UV:** Las estructuras de montaje externo, viseras y carcasas de protección de sensores deben ser fabricadas en materiales resistentes a los rayos UV (ASA/PETG).


## 3. Requerimientos Funcionales (RF)

### RF-01: Lectura Térmica Infrarroja Estratificada en Canopia
El Nodo de Campo debe medir la temperatura superficial sin contacto a través de tres estratos diferenciados de la canopia del árbol (estrato superior, medio e inferior) utilizando sensores infrarrojos MLX90614.

### RF-02: Monitoreo Ambiental y de Suelo
El Nodo de Campo debe recopilar la temperatura ambiental, humedad relativa y presión atmosférica mediante un sensor BME280, en conjunto con la humedad volumétrica (sonda capacitiva) y la temperatura del suelo.

### RF-03: Transmisión Inalámbrica de Datos de Largo Alcance
La telemetría recolectada en el Nodo de Campo debe ser transmitida utilizando señales de radio LoRa sub-GHz (banda 915 MHz) mediante un enlace punto a punto (P2P) directamente hacia el Nodo Receptor/Gateway.

### RF-04: Respaldo Local de Datos en Nodo
En caso de fallo en la transmisión inalámbrica o indisponibilidad del Gateway, el sistema debe mantener un respaldo offline de los datos:
* **Nodo de Campo:** Almacenar registros estructurados en texto/binario dentro de la memoria Flash SPI interna de 8 MB (sistema de archivos LittleFS) para evitar conflictos en el bus SPI con el módulo LoRa.
* **Nodo Gateway:** Registrar todas las tramas de telemetría entrantes en una tarjeta MicroSD offline (formato FAT32, CSV) a través de un bus SPI dedicado.

### RF-05: Ejecución por Ciclo de Trabajo (Duty-Cycle)
El Nodo de Campo debe operar bajo un ciclo de activación automatizado por tiempo, realizando el muestreo de sensores, almacenamiento local y transmisión LoRa cada 15 minutos (configurable).

## 4. Requerimientos No Funcionales (RNF)

### RNF-01: Autonomía Energética (Nodo de Campo)
* **Fuente de Alimentación:** Dos baterías Li-Ion 18650 en paralelo (capacidad nominal de 6800 mAh) recargadas mediante un panel solar mini de 5V/1W[cite: 1, 2].
* **Consumo en Reposo:** La corriente en modo *Deep Sleep* no debe superar los 0.2 mA (objetivo estimado de ~0.156 mA)[cite: 1, 2].
* **Reserva de Autonomía:** El Nodo de Campo debe mantener un funcionamiento ininterrumpido durante un mínimo de 60 días consecutivos sin radiación solar (reserva en oscuridad del 100%)[cite: 1, 2].

### RNF-02: Confiabilidad de Alimentación del Gateway
El Nodo Gateway debe operar con alimentación continua 24/7 suministrada por un adaptador de pared USB de 5V/2A[cite: 1, 2]. Debe reanudar automáticamente la escucha y el registro inmediatamente después de la restauración del suministro ante un corte eléctrico[cite: 1, 2].

### RNF-03: Integridad de Señal I2C y Extensión de Bus
* **Arquitectura de Bus:** El bus I2C principal debe multiplexarse utilizando un integrado TCA9548A/PCA9548A (dirección `0x70`)[cite: 1, 2].
* **Transmisión a Larga Distancia:** Para extender la comunicación I2C hasta 10 metros mediante cable Cat6 hacia la canopia, las líneas de señal deben ser acopladas mediante extensores de bus P82B715 operando a frecuencia reducida (100 kHz)[cite: 1, 2].

### RNF-04: Precisión de Medición y Tolerancias Térmicas
* **Temperatura Superficial Infrarroja (Follaje):** Precisión dentro de ±0.5 °C en el rango crítico de riesgo de helada (-5 °C a +5 °C)[cite: 1].
* **Temperatura Ambiental:** Precisión dentro de ±0.5 °C en todo el rango de operación (-10 °C a +45 °C)[cite: 1].


## 5. Restricciones del Sistema e Interfases Técnicas

```
+-----------------------------------------------------------------------+
|                         TOPOLOGÍA DEL SISTEMA                         |
|                                                                       |
|  NODO EMISOR DE CAMPO                              NODO GATEWAY       |
|  +------------------------+                        +---------------+  |
|  | Seeed Xiao ESP32-S3    |                        | Xiao ESP32-S3 |  |
|  | + Wio-SX1262 LoRa      |   LoRa P2P (915 MHz)   | + Wio-SX1262  |  |
|  | + TCA9548A Multiplex   |======================> |               |  |
|  | + Extensores P82B715   |  Trama Binaria 15B     | + SPI MicroSD |  |
|  | + Solar + 2x 18650 Bat |                        | + 5V/2A USB   |  |
|  +------------------------+                        +---------------+  |
+-----------------------------------------------------------------------+
```

- **Plataforma de Microcontrolador:** Seeed Studio XIAO ESP32-S3.
- **Hardware Transceptor de Radio:** Wio-SX1262 (banda Sub-GHz LoRa 915 MHz).
- **Entorno de Firmware:** PlatformIO / Framework Arduino C++.