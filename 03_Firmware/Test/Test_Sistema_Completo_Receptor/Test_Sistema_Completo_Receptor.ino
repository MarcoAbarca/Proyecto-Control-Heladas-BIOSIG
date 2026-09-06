/**
 * =============================================================================
 * Test_Sistema_Completo_Receptor.ino
 * -----------------------------------------------------------------------------
 * DEMO integrada del Receptor: escucha paquetes P2P del Emisor y los
 * muestra formateados por Serial (temperaturas, humedad, presión, suelo,
 * batería, alertas, RSSI). Sin SD ni TTN -- solo el tramo de recepción,
 * para la demo de mañana.
 *
 * Pareja: correr junto con Test_Sistema_Completo_Emisor.ino en otra placa.
 * =============================================================================
 */

#include <SPI.h>
#include <RadioLib.h>

constexpr uint8_t SPI_SCK_PIN  = D8;
constexpr uint8_t SPI_MISO_PIN = D9;
constexpr uint8_t SPI_MOSI_PIN = D10;
constexpr uint8_t LORA_CS_PIN   = D3;
constexpr uint8_t LORA_RST_PIN  = D2;
constexpr uint8_t LORA_BUSY_PIN = D1;
constexpr uint8_t LORA_DIO1_PIN = D0;

constexpr float LORA_FREQUENCY_MHZ = 915.0f;
constexpr float LORA_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;
constexpr uint8_t LORA_SYNC_WORD = 0x12;
constexpr int8_t  LORA_TX_POWER_DBM = 22; // Solo importa si este nodo tambien transmite; no afecta RX.

// DEBE coincidir exactamente con el Emisor (mismo layout que telemetry.h)
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

SX1262 radio = new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);
volatile bool packetFlag = false;
uint32_t packetsReceived = 0;

void onPacket() {
    packetFlag = true;
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // Apagado (activo en LOW en el XIAO)

    Serial.println(F("\n#####################################################"));
    Serial.println(F("#  DEMO SensAgri/Biosig - Nodo RECEPTOR            #"));
    Serial.println(F("#####################################################\n"));

    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, LORA_CS_PIN);

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("ERROR CRITICO: radio.begin() fallo (codigo %d). Revisar SPI/soldadura.\n", state);
        while (true) { delay(1000); }
    }

    radio.setFrequency(LORA_FREQUENCY_MHZ);
    radio.setBandwidth(LORA_BANDWIDTH_KHZ);
    radio.setSpreadingFactor(LORA_SF);
    radio.setCodingRate(LORA_CR);
    radio.setSyncWord(LORA_SYNC_WORD);
    radio.setOutputPower(LORA_TX_POWER_DBM);

    radio.setDio1Action(onPacket);
    radio.startReceive();

    Serial.println(F("[OK] Escuchando paquetes P2P...\n"));
}

void printFlags(uint16_t f) {
    Serial.print(F("  Sensores OK: "));
    if (f & FLAG_MLX_TOP_OK)  Serial.print(F("TOP "));
    if (f & FLAG_MLX_MID_OK)  Serial.print(F("MID "));
    if (f & FLAG_MLX_LOW_OK)  Serial.print(F("LOW "));
    if (f & FLAG_BASE_BME_OK) Serial.print(F("BME280 "));
    if (f & FLAG_SOIL_OK)     Serial.print(F("Suelo "));
    if (!(f & (FLAG_MLX_TOP_OK|FLAG_MLX_MID_OK|FLAG_MLX_LOW_OK|FLAG_BASE_BME_OK|FLAG_SOIL_OK))) {
        Serial.print(F("(ninguno)"));
    }
    Serial.println();

    if (f & FLAG_LOW_BATTERY)   Serial.println(F("  [!] Bateria baja"));
    if (f & FLAG_I2C_BUS_ERROR) Serial.println(F("  [!] Error de bus I2C en el Emisor"));
    if (f & FLAG_ALERT_FROST)      Serial.println(F("  *** ALERTA: RIESGO DE HELADA ***"));
    if (f & FLAG_ALERT_HEAT_WINTER) Serial.println(F("  *** ALERTA: SOBRECALENTAMIENTO INVERNAL ***"));
}

void loop() {
    if (!packetFlag) {
        return;
    }
    packetFlag = false;

    TelemetryPayload payload;
    int16_t state = radio.readData((uint8_t*)&payload, sizeof(payload));
    radio.startReceive(); // Siempre se re-arma, haya salido bien o mal.

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[RX] ERROR al leer paquete (codigo %d)\n", state);
        return;
    }

    packetsReceived++;
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println(F("======================================================"));
    Serial.printf("PAQUETE #%lu recibido | nodo=%d | seq=%lu | RSSI=%.1f dBm | SNR=%.1f dB\n",
                  (unsigned long)packetsReceived, payload.nodeId, (unsigned long)payload.sequenceNumber,
                  radio.getRSSI(), radio.getSNR());

    Serial.printf("  Copa TOP: %.2f C | MID: %.2f C | LOW: %.2f C\n",
                  payload.tempCanopyTop_x100 / 100.0f,
                  payload.tempCanopyMid_x100 / 100.0f,
                  payload.tempCanopyLow_x100 / 100.0f);
    Serial.printf("  Base:     %.2f C | %.1f%%RH | %.1f hPa\n",
                  payload.tempBaseC_x100 / 100.0f,
                  payload.humBasePct_x100 / 100.0f,
                  payload.pressureHpa_x10 / 10.0f);
    Serial.printf("  Suelo:    %.1f%% (sin calibrar) | Bateria: %u mV\n",
                  payload.soilMoisturePct_x100 / 100.0f, payload.batteryMilliVolts);

    printFlags(payload.statusFlags);

    delay(150);
    digitalWrite(LED_BUILTIN, HIGH);
}
