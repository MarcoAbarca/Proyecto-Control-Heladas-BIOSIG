#include <Arduino.h>
#include <RadioLib.h>

namespace {
constexpr uint8_t SPI_SCK_PIN = D8;
constexpr uint8_t SPI_MISO_PIN = D9;
constexpr uint8_t SPI_MOSI_PIN = D10;
constexpr uint8_t LORA_CS_PIN = D3;
constexpr uint8_t LORA_RST_PIN = D2;
constexpr uint8_t LORA_BUSY_PIN = D1;
constexpr uint8_t LORA_DIO1_PIN = D0;

constexpr float LORA_FREQUENCY_MHZ = 915.0f;
constexpr float LORA_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t LORA_SPREADING_FACTOR = 7;
constexpr uint8_t LORA_CODING_RATE = 5;
constexpr uint8_t LORA_SYNC_WORD = 0x12;
constexpr int8_t LORA_TX_POWER_DBM = 22;
constexpr uint16_t LORA_PREAMBLE_LENGTH = 8;

SX1262 radio(new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN));
volatile bool packetReceived = false;

void onPacketReceived() {
    packetReceived = true;
}

bool configureRadio() {
    int16_t state = radio.begin(
        LORA_FREQUENCY_MHZ,
        LORA_BANDWIDTH_KHZ,
        LORA_SPREADING_FACTOR,
        LORA_CODING_RATE,
        LORA_SYNC_WORD,
        LORA_TX_POWER_DBM,
        LORA_PREAMBLE_LENGTH
    );

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA-TEST] radio.begin() fallo: %d\n", state);
        Serial.println(F("[LORA-TEST] Revisar asentamiento B2B, alimentacion y SX1262."));
        return false;
    }

    Serial.println(F("[LORA-TEST] SX1262 detectado y configurado."));
    return true;
}
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n=== TEST AISLADO SX1262 ==="));
    Serial.println(F("Pinout: CS=D3, DIO1=D0, RST=D2, BUSY=D1, SPI=D8/D9/D10"));
    Serial.println(F("Radio: 915 MHz, BW 125 kHz, SF7, CR 4/5, sync 0x12"));

    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, LORA_CS_PIN);

    if (!configureRadio()) {
        return;
    }

#ifdef LORA_TEST_RX
    radio.setDio1Action(onPacketReceived);
    int16_t state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA-TEST] startReceive() fallo: %d\n", state);
        return;
    }
    Serial.println(F("[LORA-TEST] RX activo. Esperando paquetes..."));
#elif defined(LORA_TEST_TX)
    Serial.println(F("[LORA-TEST] TX activo. Enviando cada 2 segundos..."));
#else
    Serial.println(F("[LORA-TEST] Error: seleccionar lora_tx o lora_rx."));
#endif
}

void loop() {
#ifdef LORA_TEST_RX
    if (packetReceived) {
        packetReceived = false;
        uint8_t buffer[64] = {};
        size_t length = radio.getPacketLength();
        int16_t state = radio.readData(buffer, sizeof(buffer));

        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("[LORA-TEST] RX OK: %u bytes, RSSI=%d dBm, SNR=%.1f dB\n",
                          static_cast<unsigned>(length),
                          static_cast<int>(radio.getRSSI()),
                          radio.getSNR());
            Serial.print(F("[LORA-TEST] Datos: "));
            for (size_t index = 0; index < length && index < sizeof(buffer); index++) {
                Serial.printf("%02X ", buffer[index]);
            }
            Serial.println();
        } else {
            Serial.printf("[LORA-TEST] readData() fallo: %d\n", state);
        }

        radio.startReceive();
    }
#elif defined(LORA_TEST_TX)
    static uint32_t sequence = 0;
    char message[32];
    snprintf(message, sizeof(message), "LORA_TEST_%lu", static_cast<unsigned long>(sequence++));
    int16_t state = radio.transmit(message);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA-TEST] TX OK: %s\n", message);
    } else {
        Serial.printf("[LORA-TEST] TX fallo: %d\n", state);
    }
    delay(2000);
#else
    delay(1000);
#endif
}
