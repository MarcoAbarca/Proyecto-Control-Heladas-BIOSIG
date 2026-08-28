/**
 * =============================================================================
 * config.h
 * -----------------------------------------------------------------------------
 * Configuración centralizada para AMBOS roles (Emisor/Receptor). Las
 * secciones específicas de un rol están marcadas y comentadas; se dejan
 * definidas para ambos binarios (no cuesta nada tener una constante sin
 * usar) salvo que se indique lo contrario.
 *
 * NOTA DE VERIFICACIÓN DE HARDWARE: los pines SPI del módulo LoRa y de la
 * MicroSD son una PROPUESTA a partir de tu documento ("CS en D3/GPIO3 o
 * D2/GPIO2"), completada con RST/DIO1/BUSY porque el chip es SX1262 (no
 * SX127x). Verificalos contra el cableado físico real antes de flashear.
 * =============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// 1. PINOUT - BUS I2C PRINCIPAL (común, ya validado en fases anteriores)
// =============================================================================
constexpr uint8_t I2C_SDA_PIN = 4;   // D4 -> SDA del PCA9548A
constexpr uint8_t I2C_SCL_PIN = 5;   // D5 -> SCL del PCA9548A

// =============================================================================
// 2. DIRECCIONES I2C Y ASIGNACIÓN DEFINITIVA DE CANALES (PCA9548A: 0x70)
// =============================================================================
constexpr uint8_t PCA9548A_ADDR = 0x70;
constexpr uint8_t PCA9548A_CHANNEL_COUNT = 8;

// --- Canal 0: Bus extendido (P82B715 + 2m Cat6) -> sensor EXTERNO ----------
constexpr uint8_t CH_EXT_LONGBUS = 0;
constexpr uint8_t ADDR_SENSOR_EXTERNO = 0x44; // SHT3x por defecto (0x76 si es BME280 en su lugar)

// --- Canal 1: BME280 local (temp/hum/presión, base/suelo) -----------------
constexpr uint8_t CH_LOCAL_BME280 = 1;
constexpr uint8_t ADDR_BME280_LOCAL = 0x76;

// --- Canal 2: MPU6050 (inclinación/estructura) -----------------------------
constexpr uint8_t CH_LOCAL_MPU6050 = 2;
constexpr uint8_t ADDR_MPU6050_LOCAL = 0x68;

// --- Canales 3 a 7: reservados para expansión ------------------------------
constexpr uint8_t CH_LOCAL_3 = 3;
constexpr uint8_t CH_LOCAL_4 = 4;
constexpr uint8_t CH_LOCAL_5 = 5;
constexpr uint8_t CH_LOCAL_6 = 6;
constexpr uint8_t CH_LOCAL_7 = 7;

// =============================================================================
// 3. FRECUENCIAS DE BUS I2C
// =============================================================================
constexpr uint32_t I2C_FREQ_LOCAL_HZ    = 400000UL; // Canales 1-7 (PCB local)
constexpr uint32_t I2C_FREQ_EXTENDED_HZ = 100000UL; // Canal 0 (P82B715/Cat6)

// =============================================================================
// 4. TEMPORIZACIÓN Y TIMEOUTS
// =============================================================================
constexpr uint32_t SENSOR_READ_TIMEOUT_MS      = 500;
constexpr uint32_t I2C_TRANSACTION_TIMEOUT_MS  = 100;
constexpr uint32_t READ_CYCLE_INTERVAL_MS      = 60000; // 1 ciclo/minuto (Emisor)

// =============================================================================
// 5. RECUPERACIÓN DE BUS I2C
// =============================================================================
constexpr uint8_t  I2C_RECOVERY_CLOCK_PULSES   = 9;
constexpr uint16_t I2C_RECOVERY_PULSE_DELAY_US = 5;

// =============================================================================
// 6. SERIAL / DEPURACIÓN
// =============================================================================
constexpr uint32_t SERIAL_BAUD_RATE     = 115200;
constexpr uint32_t SERIAL_BOOT_DELAY_MS = 1500; // Retirar en firmware de campo final

// =============================================================================
// 7. RADIO LoRa - CAPA FÍSICA COMÚN (SX1262 sobre SPI nativo)
// =============================================================================
// Bus SPI compartido (hardware FSPI del XIAO ESP32-S3). Se comparte entre
// el radio LoRa y (en el Receptor) la MicroSD; cada dispositivo tiene su
// propio pin CS, el resto de las líneas son comunes al bus.
constexpr uint8_t SPI_SCK_PIN  = 8;  // D8
constexpr uint8_t SPI_MISO_PIN = 9;  // D9
constexpr uint8_t SPI_MOSI_PIN = 10; // D10

// Pines dedicados del SX1262. IMPORTANTE: el SX1262 usa DIO1 + BUSY, no
// DIO0 (eso es de la familia SX127x). CS = D3 según tu propuesta.
constexpr uint8_t LORA_CS_PIN   = 3;  // D3 (GPIO3) - Chip Select del SX1262
constexpr uint8_t LORA_RST_PIN  = 6;  // D6 (GPIO6) - Reset
constexpr uint8_t LORA_DIO1_PIN = 7;  // D7 (GPIO7) - IRQ (equivalente a "DIO0" en SX127x)
constexpr uint8_t LORA_BUSY_PIN = 2;  // D2 (GPIO2) - Línea BUSY, exclusiva de SX126x/SX128x

// =============================================================================
// 8. LoRa P2P (Emisor <-> Receptor) - parámetros de radio compartidos
// =============================================================================
// Deben ser IDÉNTICOS en Emisor y Receptor para que se escuchen entre sí.
// 915.0 MHz corresponde a la banda AU915/US915 (Chile); confirmar el plan
// de canales/sub-banda si en algún momento se valida contra normativa local.
constexpr float    LORA_P2P_FREQUENCY_MHZ = 915.0f;
constexpr float    LORA_P2P_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t  LORA_P2P_SPREADING_FACTOR = 9;   // SF9: balance alcance/tiempo-en-aire
constexpr uint8_t  LORA_P2P_CODING_RATE = 5;        // 4/5
constexpr uint8_t  LORA_P2P_SYNC_WORD = 0x12;       // Red privada (0x34 = LoRaWAN público)
constexpr int8_t   LORA_P2P_TX_POWER_DBM = 17;
constexpr uint16_t LORA_P2P_PREAMBLE_LENGTH = 8;

// Identificador de este nodo emisor dentro del payload (permite distinguir
// múltiples nodos de campo en el mismo Receptor). Cambiar por nodo físico.
constexpr uint8_t NODE_ID = 1;

// =============================================================================
// 9. LoRaWAN / TTN - SOLO usado por el Receptor como puente (OTAA)
// =============================================================================
// ⚠️ Completar con las credenciales reales generadas en la consola de TTN
// para el dispositivo "puente" (el Receptor es el único que se une a TTN).
// Formato de bytes: MSB primero (como los muestra la consola de TTN al
// elegir formato "MSB").
constexpr uint8_t TTN_DEV_EUI[8]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr uint8_t TTN_JOIN_EUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // AppEUI
constexpr uint8_t TTN_APP_KEY[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Región/plan de frecuencias LoRaWAN para el join OTAA a TTN. Debe coincidir
// con el gateway TTN más cercano (en Chile: AU915, sub-banda 2 es lo más
// común para TTN Community Edition).
#define TTN_REGION_AU915

// =============================================================================
// 10. MicroSD - SOLO usado por el Receptor (bus SPI compartido, ver Sección 7)
// =============================================================================
constexpr uint8_t  SD_CS_PIN = 1; // D1 (GPIO1). D0/GPIO0 se evita: pin de
                                   // strapping del ESP32-S3, riesgoso en boot.
constexpr const char* SD_LOG_FILENAME_PREFIX = "/telemetria_"; // + fecha/nodo + ".csv"

#endif // CONFIG_H
