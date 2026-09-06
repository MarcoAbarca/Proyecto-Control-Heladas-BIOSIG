# Presupuesto Energético y Autonomía: Nodo Emisor de Campo

## 1. Perfil de Alimentación y Fuente Energética
* **Fuente Principal:** Banco de 2 baterías Li-Ion 18650 en paralelo (3400 mAh c/u) = 6800 mAh totales (3.7V nominal)[cite: 3].
* **Generación Solar:** 1 Mini panel solar monocristalino de 5V / 1W (200 mA pico) conectado a módulo cargador TP4056 con diodo de protección 1N5817[cite: 3].
* **Estrategia de Operación:** Ciclos de 15 minutos (900 segundos) gestionados por máquina de estados con reposo en Deep Sleep[cite: 3].
* **Respaldo Local:** Memoria Flash SPI interna de 8 MB del Seeed Studio XIAO ESP32-S3 (sistema de archivos LittleFS) para evitar colisiones en el bus SPI con el transceptor LoRa SX1262[cite: 3].

## 2. Desglose de Consumos por Estado

### A. Estado Deep Sleep (895 segundos / 99.44% del tiempo)
* XIAO ESP32-S3 (Deep Sleep): ~25 uA[cite: 3]
* Módulo LoRa SX1262 (Sleep): ~1.6 uA[cite: 3]
* Multiplexor PCA9548A + 3x Sensores MLX90614 (Standby): ~20 uA[cite: 3]
* Sensor BME280 + Sonda Capacitiva de Suelo (Standby): ~10 uA[cite: 3]
* 6x Extensores de Bus P82B715 (Quiescent Current): ~100 uA[cite: 3]
* **Corriente Total en Reposo ($I_{\text{sleep}}$): ~0.156 mA** (Alimentación directa a 3.3V sin elevador Step-Up)[cite: 3].

### B. Estado Activo (5 segundos / 0.56% del tiempo)
* XIAO ESP32-S3 + Sensores I2C + Escritura Flash LittleFS: ~80 mA[cite: 3]
* Transmisión LoRa SX1262 (Ráfaga TX a +22 dBm): ~120 mA[cite: 3]
* **Corriente Total Pico en Activo ($I_{\text{active}}$): 200 mA**[cite: 3].

## 3. Balance Energético y Autonomía

### Corriente Promedio Ponderada ($I_{\text{prom}}$)
$$I_{\text{prom}} = (200\text{ mA} \times 0.00556) + (0.156\text{ mA} \times 0.99444) = \mathbf{1.267\text{ mA}}$$[cite: 3]

### Consumo Diario Total
$$\text{Consumo Diario} = 1.267\text{ mA} \times 24\text{ h} = \mathbf{30.41\text{ mAh / día}}$$[cite: 3]

### Autonomía Teórica en Oscuridad Absoluta (Descarga al 80% = 5440 mAh)
$$\text{Autonomía} = \frac{5440\text{ mAh}}{30.41\text{ mAh/día}} \approx \mathbf{178.9\text{ Días (\approx 6 Meses)}}$$[cite: 3]

### Generación Solar Estimada (Invierno Pesimista - 2.5 Horas Sol Pico)
$$\text{Generación Diaria} = 200\text{ mA} \times 2.5\text{ h} = \mathbf{500\text{ mAh / día}}$$[cite: 3]

## 4. Veredicto Energético
Existe un superávit diario neto de **+469.59 mAh/día**[cite: 3]. El panel solar genera más de 16 veces la energía que el nodo de campo consume diariamente en condiciones operativas reales[cite: 3].