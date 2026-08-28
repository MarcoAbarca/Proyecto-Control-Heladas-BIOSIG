# 🍇 Sistema IoT de Monitoreo Térmico Foliar y Suelo (Control Heladas - BIOSIG)

Bienvenido a la bóveda de documentación técnica del sistema IoT para el monitoreo de gradiente térmico de canopia (3 estratos) y suelo, enfocado en la prevención y alerta de heladas agrotécnicas.

---

## 📌 Navegación Rápida por la Documentación

### 01. Especificaciones
* [[01_Especificaciones/SRS_Requerimientos_Sistema|Especificación de Requerimientos del Sistema (SRS)]]: Requerimientos funcionales, no funcionales, entorno operativo y límites ambientales.
* [[01_Especificaciones/Cotizacion_Agosto2026|Cotización Agosto 2026]]: Presupuestos y costos de componentes para los nodos.
* [[01_Especificaciones/Manual_de_Operacion_y_Mantenimiento|Manual de Operación y Mantenimiento]]: Guía operativa para despliegue y servicio en terreno.

---

### 02. Arquitectura de Hardware
* [[02_Arquitectura_Hardware/BOM_Lista_Materiales|BOM - Lista de Materiales]]: Listado oficial de componentes electrónicos y accesorios.
* [[02_Arquitectura_Hardware/01_Guia_de_Ensamblado_Nodo_Emisor|Guía de Ensamblado del Nodo Emisor]]: Instrucciones de ensamble físico e integración en gabinete IP65.
* [[02_Arquitectura_Hardware/Pines_y_Bus_I2C|Pinout y Bus I2C]]: Asignación de GPIOs, direcciones I2C y extensores P82B715.
* [[02_Arquitectura_Hardware/Presupuesto_Energetico_Nodo_Emisor|Presupuesto Energético del Nodo Emisor]]: Análisis de consumo, baterías 18650 y ciclo solar.
* [[02_Arquitectura_Hardware/Presupuesto_Energetico_Nodo_Gateway|Presupuesto Energético del Nodo Gateway]]: Especificaciones de alimentación continua 24/7.
* [[02_Arquitectura_Hardware/Guia_de_Configuracion_PlatformIO|Guía de Configuración de PlatformIO]]: Configuración de entorno de desarrollo y dependencias.

---

### 03. Protocolos de Datos
* [[03_Protocolos_Datos/ICD_Payload_LoRa|Estructura del Payload LoRa (ICD)]]: Definición del formato binario de 15 bytes transmitido a 915 MHz.
* [[03_Protocolos_Datos/Estructura_Archivos_MicroSD|Estructura de Archivos en MicroSD]]: Formato de almacenamiento offline, tablas CSV y respaldos LittleFS.

---

### 04. Firmware y Software
* [[04_Firmware_y_Software/FSD_Especificacion_Firmware|Especificación de Firmware (FSD)]]: Arquitectura del firmware C++, manejo de Deep Sleep, I2C y SPI.

---

### 05. Gestión y Cotizaciones
* [[05_Gestion_y_Cotizaciones/Hoja_de_Ruta_y_Cronograma|Hoja de Ruta y Cronograma]]: Hitos del proyecto, etapas de desarrollo y tiempos de entrega.

---

## 📂 Estructura del Repositorio de Trabajo

```text
01_Documentacion/
├── Indice.md
├── 00_Meta/
│   ├── Adjuntos/
│   ├── Attachments/
│   └── Plantillas/
├── 01_Especificaciones/
│   ├── Cotizacion_Agosto2026.md
│   ├── Manual_de_Operacion_y_Mantenimiento.md
│   └── SRS_Requerimientos_Sistema.md
├── 02_Arquitectura_Hardware/
│   ├── 01_Guia_de_Ensamblado_Nodo_Emisor.md
│   ├── BOM_Lista_Materiales.md
│   ├── Guia_de_Configuracion_PlatformIO.md
│   ├── Pines_y_Bus_I2C.md
│   ├── Presupuesto_Energetico_Nodo_Emisor.md
│   └── Presupuesto_Energetico_Nodo_Gateway.md
├── 03_Protocolos_Datos/
│   ├── Estructura_Archivos_MicroSD.md
│   └── ICD_Payload_LoRa.md
├── 04_Firmware_y_Software/
│   └── FSD_Especificacion_Firmware.md
└── 05_Gestion_y_Cotizaciones/
    └── Hoja_de_Ruta_y_Cronograma.md