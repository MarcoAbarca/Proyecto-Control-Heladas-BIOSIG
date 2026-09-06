#### 1. Mapeo de Pines (Nodo Emisor) - Seeed XIAO ESP32-S3 + Wio-SX1262
> Pinout fijo por el conector B2B del kit oficial. El Receptor NO usa I2C (sin sensores); su D4 se reutiliza como CS de la MicroSD.

|Pin|GPIO real|Función|
|---|---|---|
|D0|GPIO1|LoRa DIO1 (IRQ)|
|D1|GPIO2|LoRa BUSY|
|D2|GPIO3|LoRa RST|
|D3|GPIO4|LoRa NSS/CS|
|D4|GPIO5|I2C SDA (Emisor) / SD CS (Receptor)|
|D5|GPIO6|I2C SCL|
|D6|GPIO43|ADC Suelo (capacitivo) — libre de UART por usar USB-CDC nativo|
|D7|GPIO44|ADC Batería — libre de UART por usar USB-CDC nativo|
|D8|GPIO7|SPI SCK (LoRa, y SD en Receptor)|
|D9|GPIO8|SPI MISO|
|D10|GPIO9|SPI MOSI|

*(Corrección: la versión anterior de este documento asignaba D0-D2 a lecturas ADC de suelo/batería/D3 "reservado" y D6/D7 a UART — eso ignoraba que D0-D3 están tomados por el radio LoRa del kit oficial. El ADC real vive en D6/D7, únicos pines libres tras I2C y radio.)*

#### 2. Topología del Bus I2C (Emisor)
```
XIAO ESP32-S3 (D4=SDA, D5=SCL)
        │
        ▼
  PCA9548A (0x70)
   │   │   │   │
  Ch0 Ch1 Ch2 Ch3
   │   │   │   └─ BME280 base (0x76), local, 400kHz
   │   │   └─ P82B715 → Cat6 2m → MLX90614 Low (0x5A), 100kHz
   │   └─ P82B715 → Cat6 2m → MLX90614 Mid (0x5A), 100kHz
   └─ P82B715 → Cat6 2m → MLX90614 Top (0x5A), 100kHz
```
*(Corrección: el diagrama anterior mostraba BME280 en Canal 0 y solo 2 MLX90614 — contradecía la tabla del propio documento y el código. Los 3 MLX90614 comparten dirección 0x5A; se aíslan por canal, nunca dos activos a la vez.)*

#### 3. Canales del Multiplexor (PCA9548A, 0x70)
|Canal|Dispositivo|Dirección|Frecuencia|Cable|
|---|---|---|---|---|
|0|MLX90614 Top (vía P82B715)|0x5A|100kHz|Cat6 2m|
|1|MLX90614 Mid (vía P82B715)|0x5A|100kHz|Cat6 2m|
|2|MLX90614 Low (vía P82B715)|0x5A|100kHz|Cat6 2m|
|3|BME280 base|0x76|400kHz|Local <10cm|
|4-7|Reservados|—|—|—|

*(Nota de nombre: el código y el driver usan "PCA9548A"; "TCA9548A" es un chip compatible pin-a-pin y misma familia de direcciones — no afecta al firmware, pero se estandariza a PCA9548A en el resto de la documentación.)*

#### 4. Extensión con P82B715
Buffer de corriente, soporta hasta ~3000pF de bus (vs ~400pF estándar). Pull-up local (lado 3.3V, en el P82B715 de la placa): 4.7kΩ. Pull-up en el tramo extendido (lado Cat6, 5V): 330-470Ω (hasta 550Ω).

#### 5. Tabla de Direcciones I2C
|Dispositivo|Dirección|Vía|
|---|---|---|
|PCA9548A|0x70|Bus directo (D4/D5)|
|MLX90614 Top|0x5A|Canal 0|
|MLX90614 Mid|0x5A|Canal 1|
|MLX90614 Low|0x5A|Canal 2|
|BME280|0x76|Canal 3|
