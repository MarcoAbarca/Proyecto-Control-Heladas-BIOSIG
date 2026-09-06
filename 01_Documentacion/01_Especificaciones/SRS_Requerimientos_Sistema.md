# System Requirements Specification (SRS)
**EMPRESA BIOSIG — DEPARTAMENTO DE INGENIERÍA Y DESARROLLO IoT**  
**Nombre del Sistema:** Sistema IoT de Monitoreo Térmico de Gradiente Foliar y Suelo para la Gestión y Alerta Temprana de Heladas Agrotécnicas (Control Heladas - BIOSIG)  
**Código de Documento:** SRS-SENSAGRI-2026-V2  
**Fecha de Actualización:** 31 de Agosto de 2026  

---

## 1. Visión General, Propósito y Alcance del Sistema

### 1.1. Propósito
El presente documento define los requerimientos funcionales, de hardware, ambientales, energéticos y de desempeño para el desarrollo e implementación del sistema de telemetría agrotécnica de precisión. El objetivo principal es la detección continua del gradiente térmico foliar en múltiples estratos de la canopia del árbol, la caracterización microclimática ambiental y la medición de parámetros edáficos para la gestión y emisión automatizada de alertas tempranas ante eventos de helada agrotécnica o estrés térmico invernal.

### 1.2. Alcance
El sistema opera bajo una arquitectura distribuida de dos niveles:
1. **Nodo Emisor de Campo:** Desplegado en el huerto agrícola, responsable de la adquisición de datos multisensoriales, filtrado local, respaldo redundante en memoria Flash interna y transmisión inalámbrica mediante radiofrecuencia LoRa P2P a 915 MHz bajo un ciclo de trabajo (*Duty-Cycle*) optimizado por reposo profundo (*Deep Sleep*).
2. **Nodo Receptor / Gateway Concentrador:** Estación base fija ubicada en galpón o centro de control, encargada de la recepción activa de tramas P2P, almacenamiento físico local en tarjeta MicroSD (formato CSV) y retransmisión de la telemetría mediante protocolo LoRaWAN (OTAA) hacia la red pública/regional de The Things Network (TTN) para su ingesta en base de datos PostgreSQL (`sensagri`) y disparo de alertas por WhatsApp API y aplicación móvil.

---

## 2. Entorno Operativo, Límites Ambientales y Grados de Protección

### 2.1. Rangos Operativos de Temperatura y Humedad
* **Rango Térmico de Operación en Campo:** $-10.0\,\text{^\circ C}$ a $+45.0\,\text{^\circ C}$ a la intemperie en ambientes agrícolas.
* **Humedad Relativa de Operación:** $10\%$ a $100\%$ HR (con capacidad de resistencia a condensación continua y rocío).

### 2.2. Encapsulado y Protección Contra Ingreso (IP Rating)
* **Gabinete de Control Principal (Nodo Emisor y Gateway):** Grado de protección **IP65** (hermético al polvo y resistente a chorros de agua a baja presión desde cualquier ángulo).
* **Prensaestopas de Conexión Externa:** Clasificación **IP68** con rosca de apriete mecánico PG7 y PG9 con sellos de goma sintética para la salida de cables Cat6, panel solar y sonda edáfica.
* **Sondas y Encapsulados de Canopia:** Los sensores infrarrojos y conectores distribuidos en la canopia deben contar con sellado termocontraíble e impermeabilización contra lluvia directa.

### 2.3. Resistencia a la Intemperie y Radiación UV
* **Materiales de Estructura y Soporte:** Las estructuras de montaje externo, viseras de sombra, rejillas de protección de sensores y soportes mecánicos expuestos al sol deben ser fabricados exclusivamente en termoplásticos con resistencia a la degradación UV y estabilidad térmica (**Filamento ASA o PETG**). Queda prohibido el uso de PLA para elementos de campo.

---

## 3. Requerimientos Funcionales (RF)

### RF-01: Lectura Térmica Infrarroja Estratificada en Canopia
El Nodo Emisor de Campo debe medir la temperatura superficial sin contacto mecánico en tres estratos verticales diferenciados del árbol mediante sensores térmicos infrarrojos Melexis MLX90614 (módulos GY-906 a 3.3V):
* **Estrato Superior (Copa Alta):** Medición de radiación térmica en yemas expuestas a la intemperie superior.
* **Estrato Medio (Copa Media):** Medición en la zona de mayor densidad foliar.
* **Estrato Inferior (Copa Baja):** Medición a una altura aproximada de $1.0\,\text{m}$ sobre el nivel del suelo en la zona de acumulación de aire frío.

### RF-02: Monitoreo Microclimático Ambiental y Edáfico
El Nodo Emisor de Campo debe adquirir en cada ciclo operativo las siguientes variables micrometeorológicas:
* **Microclima Ambiental:** Temperatura del aire, humedad relativa y presión barométrica mediante un sensor integrado BME280.
* **Humedad del Suelo:** Humedad volumétrica mediante sonda capacitiva analógica resistente a la corrosión.
* **Temperatura de Suelo:** Temperatura en la zona de raíces ($\sim 15 - 20\,\text{cm}$ de profundidad) mediante sonda térmica digital sumergible DS18B20. ⚠️ NO implementada en el firmware actual.

### RF-03: Transmisión Inalámbrica LoRa P2P de Largo Alcance
La telemetría capturada en campo debe ser empaquetada en una trama binaria compacta (23 bytes, fijo) transmitida vía radiofrecuencia sub-GHz LoRa a 915 MHz (ancho de banda $125\,\text{kHz}$, Spreading Factor SF7 (fijo), potencia $+22\,\text{dBm}$) mediante enlace punto a punto (P2P) desde el Nodo Emisor hacia el Nodo Gateway Receptor.

### RF-04: Respaldo Local Redundante y Tolerancia a Fallos
Para prevenir la pérdida de información ante fallas en el radioenlace o caídas en la red WAN, el sistema debe implementar persistencia en dos niveles:
* **Nodo Emisor de Campo:** Registro de tramas en memoria Flash SPI interna de 8 MB del microcontrolador mediante sistema de archivos **LittleFS** (evitando el uso del bus SPI físico para no colisionar con el módulo LoRa).
* **Nodo Gateway Receptor:** Escritura inmediata de cada trama recibida en una tarjeta MicroSD externa (formato FAT32, archivo estructurado `.CSV` con rotación diaria por fecha) utilizando un bus SPI dedicado.

### RF-05: Ciclo de Trabajo Energético (Duty-Cycle)
El Nodo Emisor de Campo debe operar bajo una máquina de estados finitos (FSM) controlada por temporizador RTC, despertando cada 15 minutos (configurable), ejecutando el muestreo de sensores, almacenamiento en Flash y transmisión LoRa en un intervalo máximo de 5 segundos, retornando inmediatamente a modo *Deep Sleep*.

### RF-06: Evaluación de Umbrales Agronómicos y Banderas de Alerta
El firmware y el motor de procesamiento backend deben evaluar en tiempo real dos condiciones climáticas críticas:
* **Umbral de Riesgo de Helada Agrotécnica:** Activación de bandera de alerta cuando la temperatura de cualquier estrato foliar o ambiente sea $\le 2.0\,\text{^\circ C}$ (disparador para sistemas de mitigación como molinos de aire o riego por aspersión).
* **Umbral de Brotación Invernal Prematura:** Activación de bandera de alerta cuando la temperatura en reposo invernal supere los $> 15.0\,\text{^\circ C}$.

### RF-07: Enlace WAN y Puerta de Enlace Nube (Bridge LoRaWAN)
El Nodo Gateway Receptor debe desempaquetar la trama P2P recibida en campo y retransmitirla mediante protocolo **LoRaWAN (OTAA)** en sub-banda US915/AU915 hacia la consola de **The Things Network (TTN)**, permitiendo la ingesta a la base de datos PostgreSQL (`sensagri`) en DigitalOcean y el envío de notificaciones push y WhatsApp.

---

## 4. Requerimientos No Funcionales (RNF)

### RNF-01: Autonomía Energética del Nodo Emisor
* **Fuente de Alimentación:** Banco de 2 baterías Li-Ion 18650 en paralelo (capacidad nominal combinada de $6800\,\text{mAh}$ a $3.7\,\text{V}$) alimentadas mediante un panel solar monocristalino de $5\,\text{V} / 1\,\text{W}$ con módulo de carga TP4056 y diodo Schottky 1N5817.
* **Consumo en Reposo (*Deep Sleep*):** La corriente continua en modo reposo no debe superar los $0.2\,\text{mA}$ (valor nominal medido de $\sim 0.156\,\text{mA}$ alimentando directamente a $3.3\,\text{V}$).
* **Consumo Ponderado Diario:** Corriente promedio de $1.267\,\text{mA}$ ($\sim 30.41\,\text{mAh/día}$).
* **Reserva de Autonomía:** El sistema debe ofrecer una autonomía mínima de **178 días consecutivos de operación en oscuridad absoluta** (al $80\%$ de descarga profunda) sin recibir radiación solar.
* **Superávit Solar:** En un escenario pesimista de invierno ($2.5\text{ HSP}$), la generación solar diaria ($\sim 500\,\text{mAh/día}$) debe garantizar un superávit neto de al menos $+469.59\,\text{mAh/día}$.

### RNF-02: Confiabilidad de Alimentación del Gateway
El Nodo Gateway Receptor debe operar conectado a la red eléctrica $220\,\text{V}$ mediante adaptador USB de $5\,\text{V} / 2\,\text{A}$ en modo de escucha continua 24/7. Ante interrupciones del suministro eléctrico, debe contar con reinicio automático y reanudación de la pila LoRaWAN sin intervención manual.

### RNF-03: Integridad del Bus I2C Extendido
* **Multiplexación de Bus:** El bus I2C nativo debe aislarse mediante un multiplexor PCA9548A (dirección base `0x70`).
* **Extensión de Señal a Distancia:** Para llevar el bus I2C hacia la canopia a través de $2\,\text{m}$ a $10\,\text{m}$ de cable Ethernet Cat6 Outdoor, se deben incorporar extensores de bus bidireccionales **P82B715** en cada rama con resistencias de carga (*pull-up*) dimensionadas entre $330\,\Omega$ y $1\,\text{k}\Omega$ a $5\,\text{V}$, operando a una frecuencia de bus estandarizada de $100\,\text{kHz}$.

### RNF-04: Precisión de Medición y Tolerancias Térmicas
* **Sensores Térmicos Infrarrojos (MLX90614):** Precisión dentro de $\pm 0.5\,\text{^\circ C}$ en el rango crítico de heladas ($-5.0\,\text{^\circ C}$ a $+5.0\,\text{^\circ C}$).
* **Sensor Ambientales (BME280):** Precisión dentro de $\pm 0.5\,\text{^\circ C}$ para temperatura y $\pm 3\%$ para humedad relativa.
* **Sonda Térmica de Suelo (DS18B20):** Precisión dentro de $\pm 0.5\,\text{^\circ C}$ entre $-10\,\text{^\circ C}$ y $+85\,\text{^\circ C}$. ⚠️ Sensor no implementado en firmware actual — este requisito de precisión queda pendiente.

---

## 5. Restricciones del Sistema, Topología e Interfaces Técnicas

### 5.1. Topología del Sistema

```text
+-----------------------------------------------------------------------+
|                         TOPOLOGÍA DEL SISTEMA                         |
|                                                                       |
|  NODO EMISOR DE CAMPO                              NODO GATEWAY       |
|  +------------------------+                        +---------------+  |
|  | Seeed Xiao ESP32-S3    |                        | Xiao ESP32-S3 |  |
|  | + Wio-SX1262 LoRa      |   LoRa P2P (915 MHz)   | + Wio-SX1262  |  |
|  | + TCA9548A Multiplex   |======================> | + SPI MicroSD |  |
|  | + Extensores P82B715   |  Trama Binaria (23B)   | + TTN LoRaWAN |  |
|  | + Solar + 2x 18650 Bat |                        | + 5V/2A USB   |  |
|  +------------------------+                        +---------------+  |
+-----------------------------------------------------------------------+