/*
 * ======================================================================================
 * TEST UNITARIO: Sensor BMP280 / BME280 (Conexión Directa en Banco)
 * ARCHIVO: 03_Firmware/Test/Test_BMP280_Directo.ino
 * HARDWARE: Seeed Studio XIAO ESP32-S3
 * PINES:
 *   - Pin D4 (GPIO 5): SDA Directo
 *   - Pin D5 (GPIO 6): SCL Directo
 *   - 3.3V y GND
 * ======================================================================================
 */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h> // Funciona con la mayoría de los módulos BMP280/BME280

// Pines I2C asignados para la prueba aislada
#define PIN_SDA D4 // GPIO 5
#define PIN_SCL D5 // GPIO 6

// Instancia del sensor
Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Espera la apertura del Monitor Serie

  Serial.println("\n==================================================");
  Serial.println("  TEST UNITARIO ISOLADO: SENSOR AMBIENTAL (I2C) ");
  Serial.println("==================================================");

  // Inicializar bus I2C nativo en los pines D4 y D5 a 100 kHz
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  Serial.println("Buscando sensor en direcciones I2C 0x76 o 0x77...");

  // Intenta inicializar primero en la dirección habitual 0x76, luego en 0x77
  bool status = bme.begin(0x76, &Wire);
  if (!status) {
    status = bme.begin(0x77, &Wire);
  }

  if (!status) {
    Serial.println("❌ ERROR: No se encontró el sensor BMP280/BME280.");
    Serial.println("   -> Revisa las conexiones de VCC (3.3V), GND, SDA (D4) y SCL (D5).");
    while (1) {
      delay(500); // Se detiene aquí si no hay comunicación
    }
  }

  Serial.println("✅ ¡Sensor detectado correctamente!");
  Serial.println("--------------------------------------------------");
}

void loop() {
  // Lecturas de las variables ambientales
  float tempC = bme.readTemperature();
  float humedad = bme.readHumidity();       // Retornará 0 si el módulo es BMP280 puro
  float presionhPa = bme.readPressure() / 100.0F;

  Serial.print("Temperatura: ");
  Serial.print(tempC, 2);
  Serial.print(" °C | Humedad: ");
  Serial.print(humedad, 2);
  Serial.print(" % | Presión: ");
  Serial.print(presionhPa, 2);
  Serial.println(" hPa");

  delay(1500); // Lectura refrescada cada 1.5 segundos
}