# Presupuesto Energético y Especificación: Nodo Gateway Receptor

---
## 1. Perfil de Alimentación y Arquitectura
* **Función Operativa:** Estación central fija en oficina o galpón operando en modo de escucha activa continua (RX_CONTINUOUS)[cite: 3].
* **Fuente de Alimentación:** Adaptador de red eléctrica 220V AC a USB 5V / 2A continuo[cite: 3].
* **Estrategia de Operación:** Procesamiento activo 24/7 sin modo Deep Sleep[cite: 3].
* **Persistencia de Datos:** Almacenamiento continuo en tarjeta MicroSD externa mediante bus SPI dedicado y retransmisión por interfaz USB CDC / Serie[cite: 3].

---

## 2. Desglose de Consumo de Corriente

| Componente / Periférico | Estado de Operación | Corriente Nominal (a 5V) | Potencia Estimada |
| :--- | :--- | :--- | :--- |
| **Seeed Studio XIAO ESP32-S3** | Activo (CPU a 240 MHz + USB CDC) | $90.0\text{ mA}$ | $0.45\text{ W}$[cite: 3] |
| **Módulo LoRa Wio-SX1262** | Escucha Activa Continua (RX_CONTINUOUS) | $15.0\text{ mA}$ | $0.075\text{ W}$[cite: 3] |
| **Módulo Lector MicroSD (SPI)** | Escritura / Standby Promedio | $20.0\text{ mA}$ | $0.10\text{ W}$[cite: 3] |
| **Indicadores LED de Estado** | Activo | $5.0\text{ mA}$ | $0.025\text{ W}$[cite: 3] |
| **CONSUMO TOTAL CONTINUO** | **Escucha y Registro Activo** | **$130.0\text{ mA}$** | **$0.65\text{ W}$**[cite: 3] |

---

## 3. Balance de Consumo Diario y Costo Operativo

### Consumo de Energía Diario ($E_{\text{diaria}}$)
* **Corriente Continua Promedio:** $I_{\text{promedio}} = 130.0\text{ mA}$ (a $5\text{V DC}$)[cite: 3].
* **Consumo de Capacidad Diario:** $130.0\text{ mA} \times 24\text{ h} = \mathbf{3120.0\text{ mAh / día (a 5V)}}$[cite: 3].
* **Energía Diaria Consumida:** $0.65\text{ W} \times 24\text{ h} = \mathbf{15.6\text{ Wh / día}} = \mathbf{0.0156\text{ kWh / día}}$[cite: 3].

### Consumo Energético Mensual
$$\text{Energía Mensual (30 días)} = 0.0156\text{ kWh/día} \times 30 = \mathbf{0.468\text{ kWh / mes}}$$[cite: 3]

* **Impacto Económico:** Con una tarifa eléctrica promedio de $\$150\text{ CLP/kWh}$, el costo operativo de mantener el Gateway encendido 24/7 es inferior a **$\$71\text{ CLP}$ al mes**[cite: 3].

---

## 4. Consideraciones de Respaldo ante Cortes Eléctricos (UPS / Powerbank)
Para garantizar la continuidad operativa ante caídas del suministro eléctrico en el galpón durante tormentas o eventos de helada, el Gateway puede respaldarse mediante un Powerbank estándar de $10.000\text{ mAh}$ con soporte para *Pass-Through Charging* (carga y descarga simultánea)[cite: 3]:

$$\text{Autonomía con Powerbank 10.000 mAh (3.7V / 37Wh)} = \frac{37\text{ Wh} \times 0.85\text{ (Eficiencia Conversión)}}{0.65\text{ W}} \approx \mathbf{48.3\text{ Horas}}$$[cite: 3]

El Gateway tolera cortes de energía eléctrica continua de hasta **2 días completos (48 horas)** funcionando exclusivamente desde el respaldo de batería antes de apagarse[cite: 3].