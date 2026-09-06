/**
 * =============================================================================
 * Test_LoRa_P2P_Tx.ino
 * -----------------------------------------------------------------------------
 * Prueba independiente de transmisión LoRa P2P cruda (sin LoRaWAN/TTN) con
 * el SX1262 del kit Wio-SX1262. Transmite un mensaje corto cada 3s a
 * 915MHz / SF7 y destella el LED integrado en cada transmisión exitosa.
 *
 * Requiere: libreria "RadioLib" (Library Manager). Para verificar
 * recepción, correr este mismo sketch en un segundo XIAO+Wio-SX1262 con
 * un sketch de recepción simétrico (startReceive + readData), o usar el
 * propio Nodo Receptor del proyecto ya en modo escucha P2P.
 *
 * NOTA: LED_BUILTIN en el XIAO ESP32S3 suele ser activo en LOW (encendido
 * con digitalWrite(LOW)) -- si el LED no destella, probar invertir la
 * logica antes de asumir que la transmision fallo.
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

constexpr float    LORA_FREQUENCY_MHZ = 915.0f;
constexpr float    LORA_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t  LORA_SPREADING_FACTOR = 7;
constexpr uint8_t  LORA_CODING_RATE = 5;
constexpr uint8_t  LORA_SYNC_WORD = 0x12;
constexpr int8_t   LORA_TX_POWER_DBM = 22;

SX1262 radio = new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

uint32_t txCount = 0;

void blinkLed(uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, LOW);  // Ver nota: activo en LOW en el XIAO
        delay(80);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(80);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // Apagado (activo en LOW)

    Serial.println(F("\n=== Test LoRa P2P TX ==="));

    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, LORA_CS_PIN);

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("ERROR: radio.begin() fallo, codigo %d. Deteniendo test.\n", state);
        while (true) { delay(1000); } // Detiene el test; revisar pinout/soldadura.
    }

    radio.setFrequency(LORA_FREQUENCY_MHZ);
    radio.setBandwidth(LORA_BANDWIDTH_KHZ);
    radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
    radio.setCodingRate(LORA_CODING_RATE);
    radio.setSyncWord(LORA_SYNC_WORD);
    radio.setOutputPower(LORA_TX_POWER_DBM);

    Serial.println(F("Radio inicializado. Transmitiendo cada 3s...\n"));
}

void loop() {
    char message[32];
    snprintf(message, sizeof(message), "TEST_P2P #%lu", (unsigned long)txCount);

    int16_t state = radio.transmit((uint8_t*)message, strlen(message));

    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("[TX #%lu] OK: \"%s\"\n", (unsigned long)txCount, message);
        blinkLed(1);
    } else {
        Serial.printf("[TX #%lu] ERROR, codigo %d\n", (unsigned long)txCount, state);
        blinkLed(3); // Destello distinto para diferenciar error de exito a simple vista.
    }

    txCount++;
    delay(3000);
}
