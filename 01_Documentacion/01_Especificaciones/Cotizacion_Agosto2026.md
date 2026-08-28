## 1. Resumen Ejecutivo del Presupuesto

| **Categoría**                        | **Estado de Compra**     | **Costo Estimado (CLP)** | **Proveedores Principales**       |
| ------------------------------------ | ------------------------ | ------------------------ | --------------------------------- |
| **Procesamiento y Radio LoRa**       | Comprado / Adquirido     | $32.000                  | Seeed Studio / Distribuidor Local |
| **Sensores de Canopia y Suelo**      | Comprado / En Tránsito   | $28.500                  | Mechatronic Store / AliExpress    |
| **Bus Extendido e Integrados I2C**   | Pendiente de Adquisición | $12.400                  | Mouser / Distribuidor Local       |
| **Alimentación y Energía Solar**     | Comprado                 | $18.500                  | Sodimac / MercadoLibre            |
| **Gabinete y Estanqueidad IP65**     | Comprado / Adaptado      | $14.000                  | Retail / Sodimac                  |
| **Cables, Estructuras 3D y Pasivos** | Parcial (Cable resuelto) | $11.500                  | Ferretería / Impresión propia     |
| **COSTO TOTAL ESTIMADO**             | **Proyectado**           | **$116.900 CLP**         | —                                 |

## 2. Desglose Detallado por Componente

### 2.1 Procesamiento, Radio y Control Central

- **2x Seeed Studio XIAO ESP32-S3:** Microcontrolador principal (1 para Nodo Emisor y 1 para Gateway).
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $18.000 CLP.
        
- **2x Módulo LoRa Wio-SX1262 (915 MHz):** Transceptor de radio sub-GHz P2P.
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $14.000 CLP.
        

### 2.2 Instrumental de Sensorización (Follaje, Microclima y Suelo)

- **3x Sensor Térmico Infrarrojo MLX90614 (Módulo GY-906, versión 3.3V):** Medición de temperatura sin contacto para estratos Superior, Medio e Inferior de la canopia.
    
    - _Estado:_ Adquirido / En tránsito.
        
    - _Costo Subtotal:_ $19.500 CLP ($6.500 c/u).
        
- **1x Sensor BME280 (I2C):** Temperatura, humedad relativa y presión atmosférica en base.
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $4.500 CLP.
        
- **1x Sensor Capacitivo de Humedad de Suelo + Sonda Térmica NTC:**
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $4.500 CLP.
        

### 2.3 Electrónica de Bus Extendido (Gestión I2C)

- **1x Módulo Multiplexor I2C TCA9548A / PCA9548A:** Dirección base `0x70` para conmutación de canal.
    
    - _Estado:_ **PENDIENTE DE COMPRA**.
        
    - _Costo Estimado:_ $3.400 CLP.
        
- **6x Integrados Buffer Extensores I2C P82B715:** (3 pares para acoplar el canal local con el canal remoto en cada sensor de copa).
    
    - _Estado:_ **PENDIENTE DE COMPRA**.
        
    - _Costo Estimado:_ $9.000 CLP ($1.500 c/u).
        

### 2.4 Sistema de Alimentación y Energía

- **2x Baterías Li-Ion 18650 (3400 mAh c/u en paralelo = 6800 mAh):** Banco de alimentación nodo de campo.
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $9.000 CLP.
        
- **1x Módulo Cargador TP4056 con Protección Li-Ion:**
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $1.500 CLP.
        
- **1x Panel Solar Mini 5V / 1W:** Carga de mantenimiento diaria.
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $8.000 CLP.
        

### 2.5 Estructura Física, Gabinete y Cableado

- **1x Caja Estanca IP65 (Gabinete Central de Campo):**
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $8.500 CLP.
        
- **Prensaestopas PG9 / PG7 + O-Rings:** Sellado de salidas de cable Cat6 y sensores.
    
    - _Estado:_ Adquirido.
        
    - _Costo Subtotal:_ $5.500 CLP.
        
- **10 Metros Cable UTP Cat6 Outdoor:** Línea de transmisión del bus I2C extendido.
    
    - _Estado:_ Adquirido (Sodimac).
        
    - _Costo Subtotal:_ $5.490 CLP.
        
- **Viseras y Protectores de Sensores 3D (PETG / ASA UV):** Impresión de cubiertas para protección foliar en rama.
    
    - _Estado:_ Impresión propia / Pendiente de fabricación final.
        
    - _Costo Subtotal (Filamento):_ $6.000 CLP.
        

## 3. Observaciones y Pendientes Directos de Compra

1. **Prioridad de Adquisición:** Se debe concretar el pedido de los **6x P82B715** y el **1x TCA9548A** para cerrar la infraestructura electrónica del bus de datos.
    
2. **Material de Impresión 3D:** Asegurar el uso de PETG o ASA para las viseras de los sensores MLX90614, descartando PLA debido a la degradación por radiación solar y humedad continua en el huerto.


NOTA: Los costes son aproximados, pueden variar dependiendo del lugar de compra.