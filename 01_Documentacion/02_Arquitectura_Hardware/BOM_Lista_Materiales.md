# Bill of Materials (BOM) y Componentes Físicos

## 1. Piezas Requeridas para Impresión 3D (Filamento ASA / PETG)

| Item | Pieza / Estructura 3D | Cantidad | Material Recomendado | Función Técnica / Descripción |
| :--- | :--- | :---: | :--- | :--- |
| 1 | Soporte / Base Interna para PCB | 1 | ASA / PETG | Eleva la placa perforada del fondo del gabinete IP65 para evitar condensación e higrometría[cite: 1, 4]. |
| 2 | Soporte para Panel Solar | 1 | ASA / PETG | Estructura rígida exterior para orientación al Norte e inclinación angular del panel solar[cite: 1]. |
| 3 | Viseras / Carcasas para Sensor Infrarrojo | 3 | ASA / PETG | Encapsulado protector a 45° para sensores GY-906 y extensores P82B715 contra lluvia y UV[cite: 1, 5]. |
| 4 | Rejilla de Radiación (Pantalla Stevenson) | 1 | ASA / PETG | Pantalla de platos apilados para el sensor BME280 contra radiación solar directa y salpicaduras[cite: 1, 5]. |
| 5 | Cuna / Riel para Porta-baterías 18650 | 1 | ASA / PETG | Anclaje de fijación mecánica para el porta-baterías doble en la base del gabinete[cite: 1, 4]. |

---

## 2. Componentes Nodo Emisor (Campo / Terreno)

| Item | Componente | Cantidad | Función Técnica / Descripción |
| :--- | :--- | :---: | :--- |
| 1 | Microcontrolador Seeed Studio XIAO ESP32-S3 | 1 | Unidad de procesamiento central (Dual-Core 240 MHz) y gestión de FSM[cite: 1, 2]. |
| 2 | Módulo LoRa Wio-SX1262 (915 MHz) | 1 | Transceptor RF Sub-GHz para transmisión inalámbrica de largo alcance[cite: 1, 2]. |
| 3 | Módulo Sensor Infrarrojo GY-906 (MLX90614) | 3 | Lectura térmica foliar/yemas sin contacto (Estrato Superior, Medio e Inferior)[cite: 1, 2]. |
| 4 | Módulo Sensor Ambiental BME280 | 1 | Medición microclimática de temperatura, humedad relativa y presión barométrica[cite: 1, 2]. |
| 5 | Sensor Capacitivo de Humedad de Suelo v1.2 | 1 | Mediciones de humedad volumétrica del suelo en zona de raíces[cite: 1, 2]. |
| 6 | Sonda Térmica Digital DS18B20 ⚠️ NO IMPLEMENTADA | 1 | Sonda metálica sumergible 1-Wire. NO usada en firmware actual (05_Code solo lee humedad capacitiva por ADC). |
| 7 | Multiplexor I2C PCA9548A | 1 | Conmutador I2C de 8 canales (dirección 0x70) para aislamiento de buses[cite: 1, 2]. |
| 8 | Extensores Bus I2C P82B715 | 6 | Buffer bidireccional de corriente (3 pares) para tramos largos I2C sobre Cat6[cite: 1, 2, 4]. |
| 9 | Lector MicroSD SPI + Tarjeta MicroSD | 1 | Respaldo físico local offline (Data Logger) en formato CSV[cite: 1, 2, 4]. |
| 10 | Baterías Li-Ion 18650 (3400 mAh c/u) | 2 | Banco de alimentación Li-Ion conectado en paralelo (6800 mAh total)[cite: 1, 3]. |
| 11 | Circuito Carga / Elevador (TP4056 + Step-Up 5V) | 1 Kit | Gestión de carga solar de batería Li-Ion y regulación a 5V[cite: 1, 2, 4]. |
| 12 | Mini Panel Solar 5V (1W - 5W) | 1 | Recarga fotovoltaica continua con diodo Schottky 1N5817 de protección[cite: 1, 2, 4]. |
| 13 | Caja Estanca IP65 (150x150 mm) | 1 | Gabinete principal hermético para protección de la electrónica interna[cite: 1]. |
| 14 | Prensacables IP68 (PG7 / PG9) | 3 | Pasacables con sello mecánico para prevenir ingreso de agua o polvo[cite: 1, 2]. |
| 15 | Cable Ethernet Cat6 Outdoor | 10m | Cable multifilar con protección UV e intemperie para arnés de canopia[cite: 1, 2]. |
| 16 | Borneras a Tornillo (Terminal Blocks 5.08 mm) | 1 Kit | Bloques de conexión mecánica para desacoplar cables externos de la PCB[cite: 2, 4, 5]. |
| 17 | Placa Perforada Verde (Perfboard) y Pasivos | 1 Kit | Base de soldadura, resistencias de pull-up (4.7k/330), condensadores y borneras[cite: 2, 4]. |

---

## 3. Componentes Nodo Receptor / Gateway (Galpón / Oficina)

| Item | Componente | Cantidad | Función Técnica / Descripción |
| :--- | :--- | :---: | :--- |
| 1 | Microcontrolador Seeed Studio XIAO ESP32-S3 | 1 | Unidad de procesamiento base para concentrador receptor[cite: 1, 2]. |
| 2 | Módulo LoRa Wio-SX1262 (915 MHz) | 1 | Transceptor en escucha continua (Continuous RX) para captura de paquetes[cite: 1, 2, 4]. |
| 3 | Antena LoRa 915 MHz (Alta Ganancia) | 1 | Antena omnidireccional con conector Pigtail/SMA para recepción optima[cite: 1, 2, 4]. |
| 4 | Lector MicroSD SPI + Tarjeta MicroSD | 1 | Almacenamiento masivo redundante local (DATALOG.CSV)[cite: 1, 2, 4]. |
| 5 | Fuente de Alimentación 5V / 2A (USB) | 1 | Adaptador de pared para suministro eléctrico continuo 24/7[cite: 1, 2, 4]. |