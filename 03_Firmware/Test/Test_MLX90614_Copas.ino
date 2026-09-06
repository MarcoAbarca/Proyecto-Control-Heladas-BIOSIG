/**
 * =============================================================================
 * Test_MLX90614_Copas.ino
 * -----------------------------------------------------------------------------
 * Prueba independiente de los 3 MLX90614 (estratos de copa), conmutando
 * canal por canal en el PCA9548A. Los 3 sensores comparten la MISMA
 * dirección de fábrica (0x5A) -- se distinguen únicamente por el canal
 * activo del mux en el momento de leer, nunca dos a la vez.
 *
 * Requiere: libreria "Adafruit MLX90614 Library" (Library Manager).
 * =============================================================================
 */

#include <Wire.h>
#include <Adafruit_MLX90614.h>

constexpr uint8_t I2C_SDA_PIN = D4;
constexpr uint8_t I2C_SCL_PIN = D5;

constexpr uint8_t PCA9548A_ADDR = 0x70;
constexpr uint8_t ADDR_MLX90614 = 0x5A;

constexpr uint8_t CH_MLX_TOP = 0;
constexpr uint8_t CH_MLX_MID = 1;
constexpr uint8_t CH_MLX_LOW = 2;

constexpr uint32_t FREQ_EXTENDED_HZ = 100000UL;

Adafruit_MLX90614 mlx; // Una sola instancia: se reinicializa (begin) en cada canal.

void selectMuxChannel(uint8_t channel) {
    Wire.beginTransmission(PCA9548A_ADDR);
    Wire.write(static_cast<uint8_t>(1 << channel));
    Wire.endTransmission();
}

void closeAllChannels() {
    Wire.beginTransmission(PCA9548A_ADDR);
    Wire.write(static_cast<uint8_t>(0x00));
    Wire.endTransmission();
}

void readAndPrint(uint8_t channel, const char* label) {
    selectMuxChannel(channel);

    if (!mlx.begin(ADDR_MLX90614)) {
        Serial.printf("[%s] ERROR: MLX90614 no responde en canal %d.\n", label, channel);
    } else {
        float ambientC = mlx.readAmbientTempC();
        float objectC  = mlx.readObjectTempC();
        Serial.printf("[%s] canal=%d  objeto=%.2fC  ambiente(sensor)=%.2fC\n",
                      label, channel, objectC, ambientC);
    }

    closeAllChannels();
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(FREQ_EXTENDED_HZ);

    Serial.println(F("\n=== Test MLX90614 x3 (estratos de copa) ==="));
}

void loop() {
    readAndPrint(CH_MLX_TOP, "TOP");
    readAndPrint(CH_MLX_MID, "MID");
    readAndPrint(CH_MLX_LOW, "LOW");
    Serial.println();
    delay(2000); // Repite cada 2s -- este test SI corre en loop(), a diferencia del scanner.
}
