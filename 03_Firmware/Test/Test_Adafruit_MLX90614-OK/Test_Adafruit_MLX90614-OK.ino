#include <Wire.h>
#include <Adafruit_MLX90614.h>

// Instancia del sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200);
  
  // Espera hasta 3 segundos a que el Monitor Serie del computador se conecte
  unsigned long start = millis();
  while (!Serial && (millis() - start < 3000)) {
    delay(10);
  }

  Serial.println("\n--- INICIANDO LECTURA MLX90614 ---");
  Serial.begin(115200);
  while (!Serial) delay(10); // Esperar apertura del Monitor Serie

  Serial.println("=== Testeo Rápido MLX90614 (GY-906) ===");

  // En XIAO ESP32-S3 SDA = D4, SCL = D5
  Wire.begin(D4, D5);

  if (!mlx.begin()) {
    Serial.println("❌ ERROR: No se encontró el sensor MLX90614. Revisa las conexiones.");
    while (1);
  }

  Serial.println("✅ Sensor MLX90614 detectado correctamente.");
}

void loop() {
  float tempAmbiente = mlx.readAmbientTempC();
  float tempObjeto = mlx.readObjectTempC();

  Serial.print("Temp. Ambiente: ");
  Serial.print(tempAmbiente);
  Serial.print(" °C | Temp. Objeto (Infrarrojo): ");
  Serial.print(tempObjeto);
  Serial.println(" °C");

  delay(1000);
}