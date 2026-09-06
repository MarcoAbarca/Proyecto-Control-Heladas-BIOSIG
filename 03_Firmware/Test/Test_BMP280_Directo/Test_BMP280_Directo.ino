/*
 * ======================================================================================
 * TEST DIAGNÓSTICO: Bus I2C + Sensor BMP280 (Sin Bloqueos)
 * ======================================================================================
 */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define PIN_SDA D4 // GPIO 5
#define PIN_SCL D5 // GPIO 6

Adafruit_BME280 bme;
bool sensorEncontrado = false;

void setup() {
  Serial.begin(115200);
  
  // Espera extendida de 5 segundos para sincronizar el puerto CDC/USB de Windows
  unsigned long inicio = millis();
  while (!Serial && (millis() - inicio < 5000)) {
    delay(10);
  }

  Serial.println("\n==================================================");
  Serial.println("   DIAGNÓSTICO I2C DIRECTO - XIAO ESP32-S3        ");
  Serial.println("==================================================");

  // Inicializar bus I2C nativo
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  Serial.println("1. Escaneando bus I2C directo en D4/D5...");
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("   -> Dispositivo I2C encontrado en dirección 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("❌ ATENCIÓN: No se detectó ningún dispositivo I2C en el bus.");
  } else {
    Serial.println("✅ Escaneo del bus I2C completado.");
  }

  Serial.println("\n2. Probando inicialización de librería BME280/BMP280...");
  if (bme.begin(0x76, &Wire)) {
    Serial.println("✅ Sensor respondiendo en dirección 0x76");
    sensorEncontrado = true;
  } else if (bme.begin(0x77, &Wire)) {
    Serial.println("✅ Sensor respondiendo en dirección 0x77");
    sensorEncontrado = true;
  } else {
    Serial.println("❌ ERROR: La librería no pudo enlazar el sensor en 0x76 ni 0x77.");
  }
  Serial.println("================================================--\n");
}

void loop() {
  if (sensorEncontrado) {
    float tempC = bme.readTemperature();
    float presionhPa = bme.readPressure() / 100.0F;

    Serial.print("Temp: ");
    Serial.print(tempC, 2);
    Serial.print(" °C | Presión: ");
    Serial.print(presionhPa, 2);
    Serial.println(" hPa");
  } else {
    Serial.println("Esperando hardware... Revisa alimentacion y cables SDA/SCL.");
  }
  
  delay(2000);
}