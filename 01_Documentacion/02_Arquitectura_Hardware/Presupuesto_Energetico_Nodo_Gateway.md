## 1. Perfil de Alimentación y Arquitectura

El **Nodo Gateway (Receptor)** opera como la estación central fija en la oficina o galpón. A diferencia del nodo emisor de campo, el Gateway requiere estar en modo de escucha continua para no perder ninguna trama LoRa entrante enviada por el nodo emisor.

- **Fuente de Alimentación:** Adaptador de red eléctrica 220V AC a USB 5V / 2A continuo.
    
- **Estrategia de Operación:** Procesamiento activo 24/7 sin modo _Deep Sleep_.
    
- **Persistencia:** Almacenamiento continuo en tarjeta MicroSD externa mediante bus SPI dedicado y retransmisión por interfaz USB CDC / Serie.
    

## 2. Desglose de Consumo de Corriente

Al mantenerse en escucha activa continua, los consumos del Gateway son constantes a lo largo del tiempo:

|**Componente / Periférico**|**Estado de Operación**|**Corriente Nominal (a 5V)**|**Potencia Estimada**|
|---|---|---|---|
|**Seeed Studio XIAO ESP32-S3**|Activo (CPU a 240 MHz + USB CDC)|$90.0\text{ mA}$|$0.45\text{ W}$|
|**Módulo LoRa Wio-SX1262**|Escucha Activa Continua (`RX_CONTINUOUS`)|$15.0\text{ mA}$|$0.075\text{ W}$|
|**Módulo Lector MicroSD (SPI)**|Escritura / Standby Promedio|$20.0\text{ mA}$|$0.10\text{ W}$|
|**Indicadores LED de Estado**|Activo|$5.0\text{ mA}$|$0.025\text{ W}$|
|**CONSUMO TOTAL CONTINUO**|**Escucha y Registro Activo**|**$\mathbf{130.0\text{ mA}}$**|**$\mathbf{0.65\text{ W}}$**|

## 3. Balance de Consumo Diario y Costo Operativo

### Consumo de Energía Diario ($E_{\text{diaria}}$)

$$I_{\text{promedio}} = 130.0\text{ mA} \quad (\text{a } 5\text{V DC})$$

$$\text{Consumo de Capacidad Diario} = 130.0\text{ mA} \times 24\text{ h} = \mathbf{3120.0\text{ mAh / día (a 5V)}}$$

$$\text{Energía Diaria Consumida} = 0.65\text{ W} \times 24\text{ h} = \mathbf{15.6\text{ Wh / día}} = \mathbf{0.0156\text{ kWh / día}}$$

### Consumo Energético Mensual

$$\text{Energía Mensual (30 días)} = 0.0156\text{ kWh/día} \times 30 = \mathbf{0.468\text{ kWh / mes}}$$

> **Impacto Económico:** Con una tarifa eléctrica promedio de $\$150\text{ CLP/kWh}$, el costo operativo de mantener el Gateway encendido 24/7 es inferior a **$\$71\text{ CLP}$ al mes**.

## 4. Consideraciones de Respaldo ante Cortes Eléctricos (UPS / Powerbank)

Para garantizar la continuidad operativa ante caídas del suministro eléctrico en el galpón durante tormentas de invierno o eventos de helada, el Gateway puede ser respaldado mediante un Powerbank estándar de $10.000\text{ mAh}$ con soporte para _Pass-Through Charging_ (carga y descarga simultánea):

$$\text{Autonomía con Powerbank 10.000 mAh (3.7V / 37Wh)} = \frac{37\text{ Wh} \times 0.85\text{ (Eficiencia Conversión)}}{0.65\text{ W}} \approx \mathbf{48.3\text{ Horas}}$$

El Gateway puede tolerar cortes de energía eléctrica continua de hasta **2 días completos (48 horas)** funcionando exclusivamente desde el respaldo de batería antes de apagarse.