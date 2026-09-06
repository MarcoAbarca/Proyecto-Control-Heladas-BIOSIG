/**
 * =============================================================================
 * Test_I2C_Scanner_Multiplexer.ino
 * -----------------------------------------------------------------------------
 * Prueba independiente (Arduino IDE, sin PlatformIO) del bus I2C y el
 * PCA9548A. Escanea los 4 canales en uso y reporta qué direcciones
 * responden en cada uno. Se espera:
 *   Canal 0, 1, 2 -> 0x5A (MLX90614, mismo estrato distinto canal)
 *   Canal 3       -> 0x76 o 0x77 (BME280, según el jumper SDO del modulo)
 *   Bus raiz (sin seleccionar canal) -> 0x70 (el propio PCA9548A)
 *
 * Board: Seeed XIAO ESP32S3. Placa en Arduino IDE: "XIAO_ESP32S3".
 * =============================================================================
 */

#include <Wire.h>

// Pines replicados de config.h -- si cambia el pinout del proyecto,
// actualizar acá también (este archivo es standalone, no incluye config.h).
constexpr uint8_t I2C_SDA_PIN = D4;
constexpr uint8_t I2C_SCL_PIN = D5;

constexpr uint8_t PCA9548A_ADDR = 0x70;
constexpr uint8_t CH_MLX_TOP = 0;
constexpr uint8_t CH_MLX_MID = 1;
constexpr uint8_t CH_MLX_LOW = 2;
constexpr uint8_t CH_BME280  = 3;

constexpr uint32_t FREQ_EXTENDED_HZ = 100000UL; // Canales 0-2 (P82B715/Cat6)
constexpr uint32_t FREQ_LOCAL_HZ    = 400000UL; // Canal 3 (BME280 local)

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

void scanCurrentBus() {
    uint8_t found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("    -> dispositivo en 0x%02X\n", addr);
            found++;
        }
    }
    if (found == 0) {
        Serial.println(F("    (sin dispositivos)"));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    Serial.println(F("\n=== Test I2C Scanner + PCA9548A ==="));

    Serial.println(F("\n[Bus raiz, sin canal seleccionado] -- se espera 0x70"));
    Wire.setClock(FREQ_LOCAL_HZ);
    scanCurrentBus();

    const struct { uint8_t channel; uint32_t freq; const char* label; } tests[] = {
        {CH_MLX_TOP, FREQ_EXTENDED_HZ, "Canal 0 (MLX90614 top, 100kHz)"},
        {CH_MLX_MID, FREQ_EXTENDED_HZ, "Canal 1 (MLX90614 mid, 100kHz)"},
        {CH_MLX_LOW, FREQ_EXTENDED_HZ, "Canal 2 (MLX90614 low, 100kHz)"},
        {CH_BME280,  FREQ_LOCAL_HZ,    "Canal 3 (BME280 base, 400kHz)"},
    };

    for (auto &t : tests) {
        Serial.printf("\n[%s]\n", t.label);
        Wire.setClock(t.freq);
        selectMuxChannel(t.channel);
        scanCurrentBus();
        closeAllChannels();
    }

    Serial.println(F("\n=== Fin del test ==="));
}

void loop() {
    // Test de una sola pasada; no repite en loop().
}
