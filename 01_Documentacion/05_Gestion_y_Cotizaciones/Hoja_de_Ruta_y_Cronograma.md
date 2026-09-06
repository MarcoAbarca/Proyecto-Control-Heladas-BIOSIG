# 📅 Hoja de Ruta y Cronograma Interactivo del Proyecto

## 📈 Progreso General del Proyecto

- [x] **Etapa 1: Definición, Arquitectura y Adquisición de Hardware** _(Completado)_
    
- [x] **Etapa 2: Pruebas Aisladas en Mesa / Banco (Arduino IDE)** _(En Progreso)_
    
- [x] **Etapa 3: Integración de Firmware Modular (PlatformIO)**
    
- [x] **Etapa 4: Ensamble Físico, Cableado y Gabinete IP65**
    
- [x] **Etapa 5: Enlace LoRa, Gateway y Almacenamiento Local**
    
- [ ] **Etapa 6: Despliegue en Terreno y Validación Agrícola**
    

## 🗓️ Cronograma Detallado y Checklist Interactivo

### 🔹 Etapa 1: Definición, Arquitectura y Adquisición _(Semanas 1 - 2)_

- [x] Definir arquitectura general del sistema (Nodo Emisor Campo + Nodo Receptor Gateway).
    
- [x] Seleccionar componentes de procesador y radio (Seeed Studio XIAO ESP32-S3 + Wio-SX1262 LoRa 915 MHz).
    
- [x] Diseñar el bus I2C extendido mediante multiplexor PCA9548A y extensores P82B715 con cable Cat6.
    
- [x] Validar el presupuesto energético y la batería Li-Ion 18650 (6800 mAh + Panel Solar 1W).
    
- [x] Establecer la estructura de repositorio Git y el Vault de Obsidian (`01_Documentacion`).
    
- [x] Adquisición y compra de componentes en distribuidores (Mechatronic Store / Mouser / AliExpress).
    

### 🔹 Etapa 2: Validaciones Aisladas en Mesa (Arduino IDE) _(Semana 3)_

_Objetivo: Probar cada sensor/módulo de forma individual para descartar fallas de fábrica antes del ensamble final._

- [x] **Prueba 01 - BME280:** Verificar lectura I2C directa de temperatura, humedad y presión ambiental.
    
- [x] **Prueba 02 - MLX90614 (GY-906):** Probar sensor infrarrojo foliar vía I2C directo.
    
- [x] **Prueba 03 - Multiplexor PCA9548A:** Probar conmutación de canales I2C (`0x70`) y escaneo de direcciones.
    
- [x] **Prueba 04 - Bus Extendido (P82B715):** Medir lecturas del MLX90614 a través del tramo de 10 metros de cable Cat6.
    
- [x] **Prueba 05 - Radio LoRa SX1262:** Enviar y recibir un paquete básico "Hello World" a 915 MHz.
    
- [x] **Prueba 06 - Sonda de Suelo + SD/Flash:** Verificar lecturas analógicas/capacitivas y escritura local de archivos.
    

### 🔹 Etapa 3: Integración de Firmware Modular en PlatformIO _(Semana 4)_

_Objetivo: Consolidar el código probado en una arquitectura no bloqueante (FSM) dentro del entorno PlatformIO._

- [x] Configurar entorno PlatformIO (`platformio.ini`) con flags del XIAO ESP32-S3.
    
- [x] Crear el gestor del bus I2C (`i2c_bus`) y la librería de control del multiplexor PCA9548A.
    
- [x] Crear el módulo de empaquetado de datos en estructura binaria (`LoRaPayload` de 15 bytes).
    
- [x] Implementar la máquina de estados con rutina de **Deep Sleep** (15 min de reposo / ráfaga de 5s).
    
- [x] Programar respaldo de seguridad en memoria Flash interna (`LittleFS`).
    
- [x] Programar el firmware receptor continuo para el Gateway con volcado de datos a MicroSD (`telemetria_acumulado.csv`).
    

### 🔹 Etapa 4: Ensamble Físico, Cableado y Gabinete IP65 _(Semana 5)_

_Objetivo: Construcción del hardware final blindado contra intemperie._

- [ ] Montaje de componentes en PCB perforada / Shield base dentro de gabinete estanco IP65.
    
- [ ] Instalación de prensaestopas PG9/PG7 y sellado de cables Cat6.
    
- [ ] Confección de las 3 sondas infrarrojas para canopia (Top, Mid, Low) selladas para intemperie.
    
- [ ] Instalación del TP4056 + Banco de baterías Li-Ion 18650 en paralelo + Panel Solar 5V/1W.
    
- [ ] Pruebas de hermeticidad y estabilidad térmica de la electrónica central.
    

### 🔹 Etapa 5: Enlace LoRa P2P y Gateway Receptora _(Semana 6)_

_Objetivo: Comprobar el enlace inalámbrico a distancia real y la persistencia local._

- [ ] Instalar la estación Gateway en la oficina/galpón con alimentación continua 5V/2A.
    
- [ ] Realizar pruebas de cobertura LoRa a 915 MHz (SF7 / BW 125 kHz) a 500m, 1 km y 1.5 km con línea de vista.
    
- [ ] Verificar el porcentaje de pérdida de paquetes (PER < 2%).
    
- [ ] Confirmar la escritura continua de registros en la tarjeta MicroSD del Gateway.
    

### 🔹 Etapa 6: Despliegue en Terreno y Monitoreo de Heladas _(Semana 7)_

_Objetivo: Instalación definitiva en el huerto agrícola._

- [ ] Montaje del Nodo Emisor en el árbol seleccionado (estratificación a 1m, 2m y 3m de altura).
    
- [ ] Inserción de la sonda capacitiva en el suelo.
    
- [ ] Orientación del panel solar hacia el norte para máxima radiación.
    
- [ ] Monitoreo continuo durante 14 días para validar la autonomía del banco de baterías.
    
- [ ] Cierre de la primera versión del proyecto y firma de memoria técnica de archivo