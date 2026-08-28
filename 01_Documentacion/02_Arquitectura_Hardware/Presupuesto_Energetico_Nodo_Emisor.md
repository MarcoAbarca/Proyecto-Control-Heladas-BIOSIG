## 1. Perfil de Alimentación y Fuente Energética

- **Fuente Principal:** Banco de 2 baterías Li-Ion 18650 en paralelo ($3400\text{ mAh}$ c/u) = **$6800\text{ mAh}$ totales** ($3.7\text{V}$ nominal).
    
- **Generación Solar:** 1 Mini panel solar monocristalino de $5\text{V} / 1\text{W}$ ($200\text{ mA}$ pico) conectado a módulo cargador TP4056 con diodo de protección 1N5817.
    
- **Estrategia de Operación:** Ciclos de 15 minutos ($900\text{ segundos}$) gestionados por máquina de estados con reposo en _Deep Sleep_.
    
- **Respaldo Local:** Memoria Flash SPI interna de $8\text{ MB}$ del Seeed Studio XIAO ESP32-S3 (sistema de archivos LittleFS) para evitar colisiones en el bus SPI con el transceptor LoRa SX1262.
    

## 2. Desglose de Consumos por Estado

### A. Estado Deep Sleep ($895\text{ segundos}$ / $99.44\%$ del tiempo)

- **XIAO ESP32-S3 (Deep Sleep):** $\sim 25\ \mu\text{A}$
    
- **Módulo LoRa SX1262 (Sleep):** $\sim 1.6\ \mu\text{A}$
    
- **Multiplexor TCA9548A + 3x Sensores MLX90614 (Standby):** $\sim 20\ \mu\text{A}$
    
- **Sensor BME280 + Sonda Capacitiva de Suelo (Standby):** $\sim 10\ \mu\text{A}$
    
- **6x Extensores de Bus P82B715 (Quiescent Current):** $\sim 100\ \mu\text{A}$
    
- **Corriente Total en Reposo ($I_{\text{sleep}}$):** **$\sim 0.156\text{ mA}$** (Alimentación directa a $3.3\text{V}$ sin elevador Step-Up).
    

### B. Estado Activo ($5\text{ segundos}$ / $0.56\%$ del tiempo)

- **XIAO ESP32-S3 + Sensores I2C + Escritura Flash LittleFS:** $\sim 80\text{ mA}$
    
- **Transmisión LoRa SX1262 (Ráfaga TX a $+22\text{ dBm}$):** $\sim 120\text{ mA}$
    
- **Corriente Total Pico en Activo ($I_{\text{active}}$):** **$200\text{ mA}$**.
    

## 3. Balance Energético y Autonomía

### Corriente Promedio Ponderada ($I_{\text{prom}}$)

$$I_{\text{prom}} = (200\text{ mA} \times 0.00556) + (0.156\text{ mA} \times 0.99444) = \mathbf{1.267\text{ mA}}$$

### Consumo Diario Total

$$\text{Consumo Diario} = 1.267\text{ mA} \times 24\text{ h} = \mathbf{30.41\text{ mAh / día}}$$

### Autonomía Teórica en Oscuridad Absoluta (Descarga al 80% = $5440\text{ mAh}$)

$$\text{Autonomía} = \frac{5440\text{ mAh}}{30.41\text{ mAh/día}} \approx \mathbf{178.9\text{ Días (\approx 6 Meses)}}$$

### Generación Solar Estimada (Invierno Pesimista - $2.5\text{ Horas Sol Pico}$)

$$\text{Generación Diaria} = 200\text{ mA} \times 2.5\text{ h} = \mathbf{500\text{ mAh / día}}$$

## 4. Veredicto Energético

Existe un **superávit diario neto de $+469.59\text{ mAh/día}$**. El panel solar genera más de **16 veces** la energía que el nodo de campo consume diariamente en condiciones operativas reales.