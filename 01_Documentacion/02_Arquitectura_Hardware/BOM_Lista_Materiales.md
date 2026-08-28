# Bill of Materials (BOM) y Componentes Impresos 3D

## 1. Piezas Requeridas para Impresion 3D
| Item | Pieza / Estructura 3D                     | Cantidad | Filamento Recomendado | Descripcion / Funcion                                                      |
| :--- | :---------------------------------------- | :------: | :-------------------: | :------------------------------------------------------------------------- |
| 1    | Base / Soporte Interno para PCB           |    1     |   PETG / PLA / ASA    | Eleva la placa perforada del fondo de la caja IP65 para evitar humedad.    |
| 2    | Soporte para Panel Solar                  |    1     |      ASA / PETG       | Estructura exterior para orientar e inclinar el panel solar hacia el sol.  |
| 3    | Viseras / Carcasas para Sensor Infrarrojo |    3     |      ASA / PETG       | Protegen los sensores GY-906 y extensores P82B715 de la lluvia y rayos UV. |
| 4    | Rejilla de Radiacion (Pantalla Stevenson) |    1     |      ASA / PETG       | Protege el sensor BME280 en la base de la caja contra radiacion directa.   |
| 5    | Cuna / Riel para Porta-baterias 18650     |    1     |   PETG / PLA / ASA    | Anclaje de fijacion para el porta-baterias doble en el fondo de la caja.   |

---

## 2. Componentes Nodo Emisor (Terreno / Campo)
| Item | Componente | Cantidad | Funcion / Descripcion |
| :--- | :--- | :---: | :--- |
| 1 | Microcontrolador Seeed Studio XIAO ESP32-S3 | 1 | Unidad central de procesamiento y gestion de energia. |
| 2 | Modulo LoRa Wio-SX1262 (915 MHz) | 1 | Radiofrecuencia de largo alcance para envio de datos. |
| 3 | Modulo Sensor Infrarrojo GY-906 (MLX90614) | 3 | Medicion de temperatura foliar (Superior, Media, Inferior). |
| 4 | Modulo Sensor Ambiental BME280 | 1 | Medicion de presion barometrica, humedad y temperatura ambiente. |
| 5 | Sensor Capacitivo de Humedad de Suelo | 1 | Medicion de la humedad en el suelo agricola. |
| 6 | Multiplexor I2C TCA9548A | 1 | Conmutacion de canal I2C para gestionar multiples dispositivos. |
| 7 | Extensores Bus I2C P82B715 | 6 | Amplificacion de la senal I2C para tramos largos de cableado. |
| 8 | Lector MicroSD SPI + Tarjeta MicroSD | 1 | Respaldador de datos local (Data Logger de respaldo). |
| 9 | Baterias Li-Ion LiitoKala 18650 (3400 mAh) | 2 | Alimentacion principal conectada en paralelo. |
| 10 | Mini Panel Solar 5V (1W - 5W) | 1 | Recarga solar continua en terreno. |
| 11 | Caja Estanca IP65 (150x150 mm) | 1 | Gabinete principal de proteccion contra intemperie. |
| 12 | Prensacables IP68 (PG7 / PG9) | 3 | Pasacables sellados para evitar ingreso de agua o polvo. |
| 13 | Cable Ethernet Cat6 Outdoor | 10m | Cable con proteccion UV para la interconexion de sensores. |

---

## 3. Componentes Nodo Receptor (Estacion Oficina)
| Item | Componente | Cantidad | Funcion / Descripcion |
| :--- | :--- | :---: | :--- |
| 1 | Microcontrolador con Radio LoRa | 1 | Recepcion de tramas LoRa 915 MHz y transmision via Wi-Fi. |
| 2 | Fuente de Alimentacion 5V / 2A (USB) | 1 | Suministro continuo 24/7 para la estacion base fija. |
| 3 | Antena LoRa 915 MHz | 1 | Antena de alta ganancia para recepcion optima. |
