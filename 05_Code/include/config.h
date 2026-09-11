/**
 * =============================================================================
 * config.h -- Configuración Global del Firmware Sensagri
 * -----------------------------------------------------------------------------
 * ACTUALIZADO: Arquitectura confirmada de 3x MLX90614 (estratos de copa) +
 * BME280 base + sonda capacitiva de suelo + batería, sobre el kit oficial
 * XIAO ESP32-S3 + Wio-SX1262 (conexión B2B).
 *
 * Credenciales OTAA de TTN actualizadas para: nodo-receptor-curico-01
 * =============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// 1. PINOUT - BUS I2C PRINCIPAL
// =============================================================================
constexpr uint8_t I2C_SDA_PIN = D4;  // I2C dedicado del kit -> SDA del PCA9548A
constexpr uint8_t I2C_SCL_PIN = D5;  // I2C dedicado del kit -> SCL del PCA9548A

// =============================================================================
// 2. DIRECCIONES I2C Y ASIGNACIÓN DEFINITIVA DE CANALES (PCA9548A: 0x70)
// -----------------------------------------------------------------------------
constexpr uint8_t PCA9548A_ADDR = 0x70;
constexpr uint8_t PCA9548A_CHANNEL_COUNT = 8;

constexpr uint8_t ADDR_MLX90614 = 0x5A; // Dirección de fábrica, igual en los 3

// --- Canales 0-2: MLX90614 vía P82B715 (extendido, 100kHz) ----------------
constexpr uint8_t CH_MLX_TOP = 0; // Estrato superior de copa
constexpr uint8_t CH_MLX_MID = 1; // Estrato medio de copa
constexpr uint8_t CH_MLX_LOW = 2; // Estrato inferior de copa

// --- Canal 3: BME280 base (local, 400kHz) ----------------------------------
constexpr uint8_t CH_LOCAL_BME280 = 3;
constexpr uint8_t ADDR_BME280_LOCAL = 0x76;

// --- Canales 4 a 7: reservados para expansión ------------------------------
constexpr uint8_t CH_LOCAL_4 = 4;
constexpr uint8_t CH_LOCAL_5 = 5;
constexpr uint8_t CH_LOCAL_6 = 6;
constexpr uint8_t CH_LOCAL_7 = 7;

// =============================================================================
// 3. FRECUENCIAS DE BUS I2C
// =============================================================================
constexpr uint32_t I2C_FREQ_LOCAL_HZ    = 400000UL; // Canal 3 (BME280 en la PCB)
constexpr uint32_t I2C_FREQ_EXTENDED_HZ = 100000UL; // Canales 0-2 (P82B715/Cat6)

// =============================================================================
// 4. TEMPORIZACIÓN Y TIMEOUTS
// =============================================================================
constexpr uint32_t SENSOR_READ_TIMEOUT_MS    = 500;
constexpr uint32_t I2C_TRANSACTION_TIMEOUT_MS  = 100;
constexpr uint32_t READ_CYCLE_INTERVAL_MS      = 900000UL; // 15 minutos

// =============================================================================
// 5. RECUPERACIÓN DE BUS I2C
// =============================================================================
constexpr uint8_t  I2C_RECOVERY_CLOCK_PULSES   = 9;
constexpr uint16_t I2C_RECOVERY_PULSE_DELAY_US = 5;

// =============================================================================
// 6. SERIAL / DEPURACIÓN
// =============================================================================
constexpr uint32_t SERIAL_BAUD_RATE     = 115200;
constexpr uint32_t SERIAL_BOOT_DELAY_MS = 1500;

// =============================================================================
// 7. RADIO LoRa - CAPA FÍSICA COMÚN (Kit oficial XIAO ESP32-S3 + Wio-SX1262)
// =============================================================================
constexpr uint8_t SPI_SCK_PIN  = D8;
constexpr uint8_t SPI_MISO_PIN = D9;
constexpr uint8_t SPI_MOSI_PIN = D10;

constexpr uint8_t LORA_CS_PIN   = D3;  // NSS
constexpr uint8_t LORA_RST_PIN  = D2;  // Reset
constexpr uint8_t LORA_BUSY_PIN = D1;  // BUSY (exclusivo SX126x)
constexpr uint8_t LORA_DIO1_PIN = D0;  // IRQ

// =============================================================================
// 8. LoRa P2P (Emisor <-> Receptor)
// =============================================================================
constexpr float    LORA_P2P_FREQUENCY_MHZ = 915.0f;
constexpr float    LORA_P2P_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t  LORA_P2P_SPREADING_FACTOR = 7;
constexpr uint8_t  LORA_P2P_CODING_RATE = 5;       // 4/5
constexpr uint8_t  LORA_P2P_SYNC_WORD = 0x12;      // Red privada
constexpr int8_t   LORA_P2P_TX_POWER_DBM = 22;
constexpr uint16_t LORA_P2P_PREAMBLE_LENGTH = 8;

constexpr uint8_t NODE_ID = 1;
constexpr uint32_t JITTER_MAX_MS = 5000;

// =============================================================================
// 8b. REGLAS DE NEGOCIO - Umbrales de temperatura (protección de cultivo)
// =============================================================================
constexpr float FROST_THRESHOLD_C    = 2.0f;
constexpr float OVERHEAT_THRESHOLD_C = 15.0f;

// =============================================================================
// 9. LoRaWAN / TTN - SOLO usado por el Receptor como puente (OTAA)
// -----------------------------------------------------------------------------
// Claves asignadas en TTN Console para el dispositivo: nodo-receptor-curico-01
// =============================================================================
constexpr uint8_t TTN_DEV_EUI[8]  = { 0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x56, 0xA6 };
constexpr uint8_t TTN_JOIN_EUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Reemplaza los bytes marcados con 0x00 por la AppKey completa copiada de tu TTN Console
constexpr uint8_t TTN_APP_KEY[16] = {
    0x18, 0x70, 0xDD, 0x9F, 0x66, 0x60, 0xDC, 0xCF,
    0x36, 0x18, 0xB1, 0x73, 0x96, 0xEE, 0x65, 0xA4
};

#define TTN_REGION_AU915
constexpr uint8_t TTN_SUBBAND = 2;
constexpr uint32_t TTN_UPLINK_INTERVAL_MS = 5UL * 60UL * 1000UL;

// =============================================================================
// 10. MicroSD - SOLO usado por el Receptor
// =============================================================================
constexpr uint8_t SD_CS_PIN = D4;
constexpr const char* SD_LOG_FILENAME_PREFIX = "/telemetria_";

// =============================================================================
// 11. Sensores de suelo (SOLO Emisor)
// =============================================================================
constexpr uint8_t ADC_SOIL_MOISTURE_PIN = D6;
constexpr uint8_t SOIL_TEMPERATURE_DATA_PIN = D7;

constexpr bool SOIL_MOISTURE_ADC_AVAILABLE = false;

constexpr uint16_t SOIL_ADC_DRY_RAW = 3000;  // TODO: calibrar en campo
constexpr uint16_t SOIL_ADC_WET_RAW = 1200;  // TODO: calibrar en campo

#endif // CONFIG_H