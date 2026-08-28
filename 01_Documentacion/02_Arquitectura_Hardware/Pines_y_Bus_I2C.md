## 1. Mapeo General de Pines (Pinout) - Seeed Studio XIAO ESP32-S3

El microcontrolador **Seeed Studio XIAO ESP32-S3** actúa como la unidad central de procesamiento en el Nodo Emisor de Campo. La asignación de sus GPIOs se optimizó para aislar los buses de comunicación (I2C y SPI), prevenir colisiones de hardware y garantizar el control de energía de periféricos.

| **Pin Físico** | **Etiqueta Serigrafía** | **GPIO ESP32-S3** | **Función / Periférico Asignado**              | **Modo de Operación**          |
| -------------- | ----------------------- | ----------------- | ---------------------------------------------- | ------------------------------ |
| **Pin 1**      | `D0` / `A0`             | `GPIO 1`          | Sonda Capacitiva de Suelo (Humedad)            | Entrada Analógica (`ADC1_CH0`) |
| **Pin 2**      | `D1` / `A1`             | `GPIO 2`          | Sonda Térmica NTC de Suelo                     | Entrada Analógica (`ADC1_CH1`) |
| **Pin 3**      | `D2` / `A2`             | `GPIO 3`          | Monitor de Batería (Divisor Resistivo Voltaje) | Entrada Analógica (`ADC1_CH2`) |
| **Pin 4**      | `D3`                    | `GPIO 4`          | _Reservado / Control Secundario_               | GPIO / Digital                 |
| **Pin 5**      | `D4`                    | `GPIO 5`          | **Línea SDA - Bus I2C Principal**              | I2C Open-Drain (Bidireccional) |
| **Pin 6**      | `D5`                    | `GPIO 6`          | **Línea SCL - Bus I2C Principal**              | I2C Clock Output               |
| **Pin 7**      | `D6` / `TX`             | `GPIO 43`         | Interfaz Serie UART (Depuración / CDC)         | Salida TX                      |
| **Pin 8**      | `D7` / `RX`             | `GPIO 44`         | Interfaz Serie UART (Depuración / CDC)         | Entrada RX                     |
| **Pin 9**      | `D8` / `SCK`            | `GPIO 7`          | SPI Clock (`SCK`) - Módulo LoRa / MicroSD      | Salida SPI                     |
| **Pin 10**     | `D9` / `MISO`           | `GPIO 8`          | SPI MISO - Módulo LoRa Wio-SX1262 / MicroSD    | Entrada SPI                    |
| **Pin 11**     | `D10` / `MOSI`          | `GPIO 9`          | SPI MOSI - Módulo LoRa Wio-SX1262 / MicroSD    | Salida SPI                     |

## 2. Arquitectura y Topología del Bus I2C

Para prevenir conflictos de direcciones I2C estáticas (los sensores MLX90614 poseen dirección de fábrica `0x5A`) y permitir la extensión de la señal a lo largo de 10 metros de cable Cat6 outdoor hasta la canopia del árbol, el bus I2C se estructura en una topología jerárquica con multiplexión y búferes de corriente.

Plaintext

```
                                  +-----------------------+
                                  | XIAO ESP32-S3 (Base)  |
                                  | GPIO 4 (SDA) / 5 (SCL)|
                                  +-----------+-----------+
                                              |
                                              ▼
                                  +-----------------------+
                                  | Multiplexor TCA9548A  |
                                  | Dirección I2C: 0x70   |
                                  +---+-------+-------+---+
                                      |       |       |
             +------------------------+       |       +------------------------+
             |                                |                                |
             ▼                                ▼                                ▼
      [Canal 0: SD0/SC0]               [Canal 1: SD1/SC1]               [Canal 2: SD2/SC2]
             |                                |                                |
      +------+------+                  +------+------+                  +------+------+
      | Sensor Base |                  | P82B715 Tx  |                  | P82B715 Tx  |
      |   BME280    |                  +------+------+                  +------+------+
      |    0x76     |                         | (Cable Cat6 10m)               | (Cable Cat6 10m)
      +-------------+                         ▼                                ▼
                                       +------+------+                  +------+------+
                                       | P82B715 Rx  |                  | P82B715 Rx  |
                                       +------+------+                  +------+------+
                                              |                                |
                                       +------+------+                  +------+------+
                                       |  MLX90614   |                  |  MLX90614   |
                                       | Estrato Top |                  | Estrato Mid |
                                       |    0x5A     |                  |    0x5A     |
                                       +-------------+                  +-------------+
```

## 3. Asignación de Canales del Multiplexor TCA9548A (`0x70`)

El multiplexor **TCA9548A** conmuta de forma aislada cada canal para evitar colisiones y reducir la capacitancia total del bus.

|**Canal TCA9548A**|**Registro Mask**|**Dispositivo Conectado**|**Dirección I2C**|**Frecuencia Bus**|**Distancia Cable**|
|---|---|---|---|---|---|
|**Canal 0**|`0x01`|Sensor Ambiental BME280 (Base)|`0x76`|100 kHz|Local (< 10 cm)|
|**Canal 1**|`0x02`|Extensor P82B715 + MLX90614 (Canopia Superior)|`0x5A`|100 kHz|10 Metros (Cat6)|
|**Canal 2**|`0x04`|Extensor P82B715 + MLX90614 (Canopia Media)|`0x5A`|100 kHz|7 Metros (Cat6)|
|**Canal 3**|`0x08`|Extensor P82B715 + MLX90614 (Canopia Inferior)|`0x5A`|100 kHz|4 Metros (Cat6)|
|**Canal 4 - 7**|`0x10` - `0x80`|_Reservados para expansión_|—|—|—|

## 4. Extensión de Bus I2C con P82B715 y Resistencias Pull-Up

El integrado **P82B715** es un búfer de corriente que transforma las líneas I2C estándar de alta impedancia en una línea de baja impedancia capaz de manejar capacitancias de bus de hasta $3000\text{ pF}$, superando holgadamente el límite estándar de $400\text{ pF}$.

### Esquema de Resistencias de Elevación (_Pull-Up_)

Para garantizar la integridad del flujo de datos sin saturar los transistores internos de los sensores a 3.3V, se configuran dos niveles de resistencias pull-up:

1. **Lado Local (Bus de Baja Capacitancia - 3.3V):**
    
    - Ubicación: Placa principal (Entrada del P82B715).
        
    - Valor: **$4.7\text{ k}\Omega$** conectadas entre SDA/SCL y VCC (3.3V).
        
2. **Lado Cable Extendido (Bus de Alta Capacitancia / Corriente - Cable Cat6):**
    
    - Ubicación: Línea de transmisión del cable Ethernet ($Lx$ y $Ly$).
        
    - Valor: **$470\ \Omega$** a **$1\text{ k}\Omega$** conectadas entre la línea extendida y VCC 3.3V (proporciona la corriente requerida por el P82B715 para conmutar rápidamente los flancos de subida en cables largos).
        

## 5. Tabla Resumen de Direcciones I2C del Sistema

|**Dispositivo**|**Función**|**Dirección I2C Nativa**|**Estado de Bus**|
|---|---|---|---|
|**TCA9548A**|Multiplexor I2C Principal|`0x70`|Directo en Bus Local (`GPIO 4 / 5`)|
|**BME280**|Microclima Base|`0x76`|Conmutado vía TCA9548A Canal 0|
|**MLX90614 (Top)**|Temp. Foliar Estrato Alto|`0x5A`|Conmutado vía TCA9548A Canal 1|
|**MLX90614 (Mid)**|Temp. Foliar Estrato Medio|`0x5A`|Conmutado vía TCA9548A Canal 2|
|**MLX90614 (Low)**|Temp. Foliar Estrato Bajo|`0x5A`|Conmutado vía TCA9548A Canal 3|