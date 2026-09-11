/**
 * =============================================================================
 * Test_Sistema_Completo_Emisor.ino
 * -----------------------------------------------------------------------------
 * DEMO integrada para mostrar el sistema funcionando de punta a punta:
 * lee los 3 MLX90614 + BME280 + suelo/batería por ADC, evalúa alertas de
 * helada/sobrecalentamiento, y transmite por LoRa P2P -- todo en un loop
 * continuo (SIN Deep Sleep) cada 5 segundos, pensado para una demo en vivo
 * con alimentación por cable, no para despliegue final en campo.
 *
 * Sensores que falten (hardware aún sin terminar de soldar) se reportan
 * como "no detectado" sin frenar el resto de la demo -- si solo tenés
 * uno o dos sensores soldados todavía, igual corre y transmite lo que sí
 * hay, con las banderas de estado reflejando exactamente eso.
 *
 * TTN/LoRaWAN NO está incluido a propósito (en pausa hasta confirmación).
 * Esto es solo el tramo Emisor -> Receptor por P2P.
 *
 * Pareja: correr junto con Test_Sistema_Completo_Receptor.ino en otra placa.
 * =============================================================================
 */

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <RadioLib.h>

// =============================================================================
// Pinout (replicado de config.h -- ver ese archivo si el pinout cambió)
// =============================================================================
constexpr uint8_t I2C_SDA_PIN = D4;
constexpr uint8_t I2C_SCL_PIN = D5;

constexpr uint8_t SPI_SCK_PIN  = D8;
constexpr uint8_t SPI_MISO_PIN = D9;
constexpr uint8_t SPI_MOSI_PIN = D10;
constexpr uint8_t LORA_CS_PIN   = D3;
constexpr uint8_t LORA_RST_PIN  = D2;
constexpr uint8_t LORA_BUSY_PIN = D1;
constexpr uint8_t LORA_DIO1_PIN = D0;

// D6 y D7 NO son pines ADC válidos en XIAO ESP32-S3, los deshabilitamos
// constexpr uint8_t ADC_SOIL_MOISTURE_PIN = D6;
// constexpr uint8_t ADC_BATTERY_PIN       = D7;
// constexpr float   BATTERY_DIVIDER_RATIO = 2.0f;

// Calibración de suelo -- PLACEHOLDERS sin calibrar (ver Test_ADC_Suelo_Bateria.ino)
constexpr uint16_t SOIL_ADC_DRY_RAW = 3000;
constexpr uint16_t SOIL_ADC_WET_RAW = 1200;

constexpr uint8_t PCA9548A_ADDR = 0x70;
constexpr uint8_t ADDR_MLX90614 = 0x5A;
constexpr uint8_t ADDR_BME280_LOCAL = 0x76;
constexpr uint8_t CH_MLX_TOP = 0, CH_MLX_MID = 1, CH_MLX_LOW = 2, CH_BME280 = 3;

constexpr float LORA_FREQUENCY_MHZ = 915.0f;
constexpr float LORA_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;
constexpr uint8_t LORA_SYNC_WORD = 0x12;
constexpr int8_t  LORA_TX_POWER_DBM = 22;

constexpr float FROST_THRESHOLD_C    = 2.0f;
constexpr float OVERHEAT_THRESHOLD_C = 15.0f;
constexpr uint8_t NODE_ID = 1;

// =============================================================================
// TelemetryPayload -- DEBE coincidir byte a byte con include/telemetry.h del
// proyecto principal. Si ese struct cambia, actualizar acá también.
// =============================================================================
constexpr uint16_t FLAG_MLX_TOP_OK    = 1 << 0;
constexpr uint16_t FLAG_MLX_MID_OK    = 1 << 1;
constexpr uint16_t FLAG_MLX_LOW_OK    = 1 << 2;
constexpr uint16_t FLAG_BASE_BME_OK   = 1 << 3;
constexpr uint16_t FLAG_SOIL_OK       = 1 << 4;
constexpr uint16_t FLAG_LOW_BATTERY   = 1 << 5;
constexpr uint16_t FLAG_I2C_BUS_ERROR = 1 << 6;
constexpr uint16_t FLAG_ALERT_FROST      = 1 << 8;
constexpr uint16_t FLAG_ALERT_HEAT_WINTER = 1 << 9;

#pragma pack(push, 1)
struct TelemetryPayload {
    uint8_t  nodeId;
    uint32_t sequenceNumber;
    int16_t  tempCanopyTop_x100;
    int16_t  tempCanopyMid_x100;
    int16_t  tempCanopyLow_x100;
    int16_t  tempBaseC_x100;
    uint16_t humBasePct_x100;
    uint16_t pressureHpa_x10;
    uint16_t soilMoisturePct_x100;
    uint16_t batteryMilliVolts;
    uint16_t statusFlags;
} __attribute__((packed));
#pragma pack(pop)

// =============================================================================
// Objetos globales
// =============================================================================
Adafruit_MLX90614 mlxTop, mlxMid, mlxLow;
Adafruit_BME280 bme;
SX1262 radio = new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

uint32_t seqNumber = 0;

void selectMuxChannel(uint8_t ch, uint32_t freq) {
    Wire.setClock(freq);
    Wire.beginTransmission(PCA9548A_ADDR);
    Wire.write(static_cast<uint8_t>(1 << ch));
    Wire.endTransmission();
}

void closeAllChannels() {
    Wire.beginTransmission(PCA9548A_ADDR);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission();
    Wire.setClock(400000UL);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println(F("\n#####################################################"));
    Serial.println(F("#  DEMO SensAgri/Biosig - Nodo EMISOR (sin sleep)  #"));
    Serial.println(F("#####################################################\n"));

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, LORA_CS_PIN);

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("ERROR CRITICO: radio.begin() fallo (codigo %d). Revisar SPI/soldadura/ANTENA.\n", state);
    } else {
        radio.setFrequency(LORA_FREQUENCY_MHZ);
        radio.setBandwidth(LORA_BANDWIDTH_KHZ);
        radio.setSpreadingFactor(LORA_SF);
        radio.setCodingRate(LORA_CR);
        radio.setSyncWord(LORA_SYNC_WORD);
        radio.setOutputPower(LORA_TX_POWER_DBM);
        Serial.println(F("[OK] Radio LoRa lista."));
    }

    Serial.println(F("Iniciando ciclo de lectura cada 5s...\n"));
}

void loop() {
    TelemetryPayload payload = {};
    payload.nodeId = NODE_ID;
    payload.sequenceNumber = seqNumber;

    Serial.println(F("----------------------------------------------------"));
    Serial.printf("CICLO #%lu\n", (unsigned long)seqNumber);

    // --- MLX90614 x3 ---
    struct { uint8_t ch; const char* label; Adafruit_MLX90614* mlx; int16_t* field; uint16_t flag; } mlxSensors[] = {
        {CH_MLX_TOP, "Copa TOP", &mlxTop, &payload.tempCanopyTop_x100, FLAG_MLX_TOP_OK},
        {CH_MLX_MID, "Copa MID", &mlxMid, &payload.tempCanopyMid_x100, FLAG_MLX_MID_OK},
        {CH_MLX_LOW, "Copa LOW", &mlxLow, &payload.tempCanopyLow_x100, FLAG_MLX_LOW_OK},
    };

    for (auto &s : mlxSensors) {
        selectMuxChannel(s.ch, 100000UL);
        if (s.mlx->begin(ADDR_MLX90614)) {
            float t = s.mlx->readObjectTempC();
            if (!isnan(t)) {
                *s.field = (int16_t)(t * 100);
                payload.statusFlags |= s.flag;
                Serial.printf("  [OK] %-10s %.2f C\n", s.label, t);
            } else {
                Serial.printf("  [!!] %-10s lectura invalida (NaN)\n", s.label);
            }
        } else {
            Serial.printf("  [--] %-10s no detectado (revisar soldadura/canal %d)\n", s.label, s.ch);
        }
        closeAllChannels();
    }

    // --- BME280 base ---
    selectMuxChannel(CH_BME280, 400000UL);
    if (bme.begin(ADDR_BME280_LOCAL)) {
        float t = bme.readTemperature();
        float h = bme.readHumidity();
        float p = bme.readPressure() / 100.0f;
        if (!isnan(t) && !isnan(h) && !isnan(p)) {
            payload.tempBaseC_x100 = (int16_t)(t * 100);
            payload.humBasePct_x100 = (uint16_t)(h * 100);
            payload.pressureHpa_x10 = (uint16_t)(p * 10);
            payload.statusFlags |= FLAG_BASE_BME_OK;
            Serial.printf("  [OK] BME280 base  %.2f C | %.1f%%RH | %.1f hPa\n", t, h, p);
        }
    } else {
        Serial.println(F("  [--] BME280 base  no detectado"));
    }
    closeAllChannels();

    // --- Suelo + batería (deshabilitados en este test - D6/D7 no son ADC válidos) ---
    payload.soilMoisturePct_x100 = 0;
    payload.batteryMilliVolts = 0;
    Serial.println(F("  [OK] Suelo/Bateria deshabilitados en este test (D6/D7 no son ADC válidos)"));

    // --- Alertas ---
    float refTemp = NAN;
    if (payload.statusFlags & FLAG_MLX_TOP_OK) refTemp = payload.tempCanopyTop_x100 / 100.0f;
    else if (payload.statusFlags & FLAG_BASE_BME_OK) refTemp = payload.tempBaseC_x100 / 100.0f;

    if (!isnan(refTemp)) {
        if (refTemp <= FROST_THRESHOLD_C) { 
            payload.statusFlags |= FLAG_ALERT_FROST;
            Serial.println(F("  *** ALERTA: RIESGO DE HELADA ***"));
        }
        if (refTemp > OVERHEAT_THRESHOLD_C) {
            payload.statusFlags |= FLAG_ALERT_HEAT_WINTER;
            Serial.println(F("  *** ALERTA: SOBRECALENTAMIENTO INVERNAL ***"));
        }
    }

    // --- Transmisión LoRa P2P ---
    int16_t txState = radio.transmit((uint8_t*)&payload, sizeof(payload));
    if (txState == RADIOLIB_ERR_NONE) {
        Serial.printf("  [TX] Payload enviado (%d bytes, flags=0x%04X)\n", (int)sizeof(payload), payload.statusFlags);
    } else {
        Serial.printf("  [TX] ERROR al transmitir (codigo %d)\n", txState);
    }

    seqNumber++;
    delay(5000);
}