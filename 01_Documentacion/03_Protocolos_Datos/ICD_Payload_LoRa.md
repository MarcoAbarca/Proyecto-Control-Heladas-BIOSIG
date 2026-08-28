# Interface Control Document (ICD): Payload LoRa P2P

## 1. Parámetros de la Capa Física (Radio LoRa)

* **Protocolo:** LoRa P2P (Peer-to-Peer) / Binary Unpacked.
* **Frecuencia Operativa:** 915.0 MHz (Banda ISM para América del Sur).
* **Ancho de Banda (BW):** 125 kHz.
* **Spreading Factor (SF):** SF7 (Optimizado para mínimo *Airtime* y bajo consumo energético).
* **Coding Rate (CR):** 4/5.
* **Preamble Length:** 8 Símbolos.
* **Potencia de Transmisión (TX Power):** +22 dBm (Módem Wio-SX1262)[.
* **Tamaño Fijo del Payload:** 15 Bytes.
* **Tiempo Estimado en Aire (Airtime):** ~41.2 ms por ráfaga.

## 2. Estructura Binaria de la Trama (`struct Payload`)

Para minimizar el consumo de energía durante la ráfaga de transmisión (5 segundos activos) y optimizar el uso del canal de radio, la telemetría no se transmite en texto plano (JSON o CSV), sino como una estructura C++ empaquetada (`__attribute__((packed))`).

Los valores de temperatura y humedad flotantes (`float`) se convierten a enteros signados/no signados multiplicados por un factor de escala $100$ para preservar exactamente dos decimales sin incurrir en el costo de almacenamiento de 4 bytes de punto flotante IEEE 754.

### Mapa de Registro de Bytes

| Byte Offset | Campo de Datos | Tipo C++ | Rango / Unidad | Factor de Escala | Descripción |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | `node_id` | `uint8_t` | `1` - `255` | 1 | Identificador único del nodo de campo. |
| **1 - 2** | `msg_counter` | `uint16_t` | `0` - `65535` | 1 | Contador incremental de tramas enviadas (para detectar paquetes perdidos). |
| **3 - 4** | `temp_canopy_top` | `int16_t` | `-4000` a `12500` (-40.00 a 125.00 °C) | x100 | Sensor IR MLX90614 en Estrato Superior (Canal 1 I2C). |
| **5 - 6** | `temp_canopy_mid` | `int16_t` | `-4000` a `12500` (-40.00 a 125.00 °C) | x100 | Sensor IR MLX90614 en Estrato Medio (Canal 2 I2C). |
| **7 - 8** | `temp_canopy_low` | `int16_t` | `-4000` a `12500` (-40.00 a 125.00 °C) | x100 | Sensor IR MLX90614 en Estrato Inferior (Canal 3 I2C). |
| **9 - 10** | `temp_soil` | `int16_t` | `-2000` a `8500` (-20.00 a 85.00 °C) | x100 | Sensor NTC / Sonda Digital de Suelo. |
| **11 - 12** | `hum_soil` | `uint16_t` | `0` a `10000` (0.00 a 100.00 %) | x100 | Humedad Volumétrica Capacitiva de Suelo. |
| **13 - 14** | `vbatt_mv` | `uint16_t` | `2800` a `4200` mV | 1 | Voltaje del banco de baterías Li-Ion 18650 en milivoltios. |

**Tamaño Total:** `1 + 2 + 2 + 2 + 2 + 2 + 2 + 2 = 15 Bytes`

## 3. Declaración de la Estructura en C++ (Firmware Emisor y Receptor)

```cpp
#ifndef ICD_PAYLOAD_LORA_H
#define ICD_PAYLOAD_LORA_H

#include <stdint.h>

// Empaquetado estricto sin relleno de memoria (padding)
struct __attribute__((packed)) LoRaPayload {
    uint8_t  node_id;          // Byte 0
    uint16_t msg_counter;      // Bytes 1-2
    int16_t  temp_canopy_top;  // Bytes 3-4  (Valor real = temp_canopy_top / 100.0f)
    int16_t  temp_canopy_mid;  // Bytes 5-6  (Valor real = temp_canopy_mid / 100.0f)
    int16_t  temp_canopy_low;  // Bytes 7-8  (Valor real = temp_canopy_low / 100.0f)
    int16_t  temp_soil;        // Bytes 9-10 (Valor real = temp_soil / 100.0f)
    uint16_t hum_soil;         // Bytes 11-12(Valor real = hum_soil / 100.0f)
    uint16_t vbatt_mv;         // Bytes 13-14(Lectura directa en milivoltios)
};

#endif // ICD_PAYLOAD_LORA_H
````

## 4. Codificación en el Nodo Emisor (Ejemplo de Implementación)

C++

```
#include "ICD_Payload_LoRa.h"
#include <RadioLib.h>

LoRaPayload payload;
uint16_t global_msg_counter = 0;

void pack_and_send_telemetry() {
    payload.node_id = 1;
    payload.msg_counter = global_msg_counter++;
    
    // Mapeo y escalado de sensores
    payload.temp_canopy_top = (int16_t)(read_mlx_top() * 100.0f);
    payload.temp_canopy_mid = (int16_t)(read_mlx_mid() * 100.0f);
    payload.temp_canopy_low = (int16_t)(read_mlx_low() * 100.0f);
    payload.temp_soil       = (int16_t)(read_soil_temp() * 100.0f);
    payload.hum_soil        = (uint16_t)(read_soil_humidity() * 100.0f);
    payload.vbatt_mv        = (uint16_t)read_battery_voltage_mv();

    // Transmisión directa por bloque de memoria pura
    int state = radio.transmit((uint8_t*)&payload, sizeof(LoRaPayload));
    
    if (state == RADIOLIB_ERR_NONE) {
        // Transmisión exitosa
    }
}
```

## 5. Decodificación en el Gateway / Parser Python

El Gateway en galpón recibe los 15 bytes crudos desde el puerto serie o bus SPI y reconstruye las variables agronómicas originales.

### Decodificador en Python (`struct.unpack`)

Python

```
import struct

PAYLOAD_FORMAT = "<BHhhhhHH" !="PAYLOAD_SIZE:" "hum_soil_pct": "msg_counter": "node_id": "temp_canopy_low_c": "temp_canopy_mid_c": "temp_canopy_top_c": "temp_soil_c": "vbatt_mv": # / 100.0, 2x 4x Little-endian: PAYLOAD_SIZE="15" Se ValueError(f"Tamaño ``` bytes, de def esperaban if int16, inválido. len(raw_bytes) parse_lora_payload(raw_bytes): raise raw_bytes) recibieron return se telemetry trama uint16 uint16, uint8, unpacked="struct.unpack(PAYLOAD_FORMAT," unpacked[0], unpacked[1], unpacked[2] unpacked[3] unpacked[4] unpacked[5] unpacked[6] unpacked[7] {PAYLOAD_SIZE} {len(raw_bytes)}.") }>
```
