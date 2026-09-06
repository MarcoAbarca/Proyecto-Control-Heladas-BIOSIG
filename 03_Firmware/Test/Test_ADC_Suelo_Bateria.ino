/**
 * =============================================================================
 * Test_ADC_Suelo_Bateria.ino
 * -----------------------------------------------------------------------------
 * Prueba independiente de las 2 lecturas analógicas del Emisor: sonda
 * capacitiva de suelo (D6) y divisor de batería (D7). Imprime RAW (0-4095,
 * 12 bits) y milivolts calibrados (analogReadMilliVolts, corrige la no
 * linealidad del ADC del ESP32-S3 cerca de los extremos del rango).
 *
 * USO PARA CALIBRAR LA SONDA DE SUELO: anotar el RAW con la sonda en aire
 * (seco total) y con la sonda sumergida en un vaso de agua (saturado) --
 * esos dos valores van en config.h: SOIL_ADC_DRY_RAW / SOIL_ADC_WET_RAW.
 * =============================================================================
 */

constexpr uint8_t ADC_SOIL_MOISTURE_PIN = D6;
constexpr uint8_t ADC_BATTERY_PIN       = D7;

// Divisor resistivo de batería: 2x 100kΩ (1:1) -> V_bateria = V_adc * 2.
constexpr float BATTERY_DIVIDER_RATIO = 2.0f;

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println(F("\n=== Test ADC Suelo + Bateria ==="));
    Serial.println(F("Ctrl+C o desconectar para terminar. Lectura cada 1s.\n"));
}

void loop() {
    int soilRaw = analogRead(ADC_SOIL_MOISTURE_PIN);
    uint32_t soilMv = analogReadMilliVolts(ADC_SOIL_MOISTURE_PIN);

    int battRaw = analogRead(ADC_BATTERY_PIN);
    uint32_t battPinMv = analogReadMilliVolts(ADC_BATTERY_PIN);
    uint32_t battRealMv = static_cast<uint32_t>(battPinMv * BATTERY_DIVIDER_RATIO);

    Serial.printf(
        "SUELO  raw=%4d  mV_pin=%4lu  |  BATERIA  raw=%4d  mV_pin=%4lu  mV_real(x%.1f)=%4lu\n",
        soilRaw, (unsigned long)soilMv,
        battRaw, (unsigned long)battPinMv, BATTERY_DIVIDER_RATIO, (unsigned long)battRealMv
    );

    delay(1000);
}
