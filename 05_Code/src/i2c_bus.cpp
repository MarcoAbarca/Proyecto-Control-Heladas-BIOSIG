/**
 * =============================================================================
 * i2c_bus.cpp
 * =============================================================================
 */

#include "i2c_bus.h"
#include "config.h"

namespace I2CBus {

// -----------------------------------------------------------------------------
// begin()
// -----------------------------------------------------------------------------
void begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQ_LOCAL_HZ);

    // Timeout a nivel de periférico: si una transacción no completa dentro
    // de esta ventana, Wire aborta en vez de bloquear el procesador
    // indefinidamente esperando un ACK que nunca llega.
    Wire.setTimeOut(I2C_TRANSACTION_TIMEOUT_MS);
}

// -----------------------------------------------------------------------------
// setFrequency()
// -----------------------------------------------------------------------------
void setFrequency(uint32_t freqHz) {
    Wire.setClock(freqHz);
}

// -----------------------------------------------------------------------------
// isBusStuck()
// -----------------------------------------------------------------------------
bool isBusStuck() {
    // Se libera el periférico Wire momentáneamente para poder leer el pin
    // SDA como GPIO puro. Si un esclavo quedó a mitad de byte, mantendrá
    // SDA forzada en LOW indefinidamente.
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    delayMicroseconds(I2C_RECOVERY_PULSE_DELAY_US);
    return digitalRead(I2C_SDA_PIN) == LOW;
}

// -----------------------------------------------------------------------------
// recoverBus()
// -----------------------------------------------------------------------------
bool recoverBus() {
    Serial.println(F("[I2CBus] Iniciando rutina de recuperacion de bus..."));

    // Se libera el control por hardware del periférico Wire para poder
    // manipular SCL/SDA como GPIO durante la recuperación manual.
    Wire.end();

    pinMode(I2C_SCL_PIN, OUTPUT);
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    digitalWrite(I2C_SCL_PIN, HIGH);

    // Si SDA ya está en HIGH, no hay nada que recuperar.
    if (digitalRead(I2C_SDA_PIN) == HIGH) {
        Serial.println(F("[I2CBus] Bus ya estaba libre, no se requiere recuperacion."));
        begin();
        return true;
    }

    // Hasta 9 pulsos manuales de reloj por SCL: cubre el peor caso de un
    // esclavo detenido a mitad de un byte de 8 bits + bit de ACK. Tras
    // cada pulso se verifica si el esclavo ya soltó SDA, para no enviar
    // pulsos de más una vez liberado el bus.
    for (uint8_t i = 0; i < I2C_RECOVERY_CLOCK_PULSES; i++) {
        digitalWrite(I2C_SCL_PIN, LOW);
        delayMicroseconds(I2C_RECOVERY_PULSE_DELAY_US);
        digitalWrite(I2C_SCL_PIN, HIGH);
        delayMicroseconds(I2C_RECOVERY_PULSE_DELAY_US);

        if (digitalRead(I2C_SDA_PIN) == HIGH) {
            Serial.printf("[I2CBus] SDA liberada tras %d pulso(s).\n", i + 1);
            break;
        }
    }

    // Condición STOP manual: con SCL en HIGH, se fuerza SDA de LOW a HIGH.
    // Esto deja el bus en un estado limpio y conocido para todos los
    // esclavos, en lugar de dejarlo a mitad de una transacción fantasma.
    pinMode(I2C_SDA_PIN, OUTPUT);
    digitalWrite(I2C_SDA_PIN, LOW);
    delayMicroseconds(I2C_RECOVERY_PULSE_DELAY_US);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(I2C_RECOVERY_PULSE_DELAY_US);
    digitalWrite(I2C_SDA_PIN, HIGH);
    delayMicroseconds(I2C_RECOVERY_PULSE_DELAY_US);

    bool released = (digitalRead(I2C_SDA_PIN) == HIGH);

    // Reinicializar el periférico Wire en modo normal, ya con los pines
    // devueltos a su función alterna I2C.
    begin();

    if (released) {
        Serial.println(F("[I2CBus] Recuperacion exitosa, bus liberado."));
    } else {
        Serial.println(F("[I2CBus] ADVERTENCIA: SDA sigue atascada. Revisar hardware/cableado."));
    }

    return released;
}

// -----------------------------------------------------------------------------
// identifyDevice()
// -----------------------------------------------------------------------------
const char* identifyDevice(uint8_t address) {
    switch (address) {
        case PCA9548A_ADDR:         return "PCA9548A (Multiplexor)";
        case ADDR_BME280_PRIMARY:   return "BME280 (primario)";
        case ADDR_BME280_SECONDARY: return "BME280 (secundario)";
        case ADDR_SHT3X_PRIMARY:    return "SHT3x (primario)";
        case ADDR_SHT3X_SECONDARY:  return "SHT3x (secundario)";
        case ADDR_MPU6050_PRIMARY:  return "MPU6050 (primario)";
        case ADDR_MPU6050_SECONDARY: return "MPU6050 (secundario)";
        default:                    return "Desconocido";
    }
}

// -----------------------------------------------------------------------------
// scanBus()
// -----------------------------------------------------------------------------
uint8_t scanBus() {
    uint8_t found = 0;

    // Rango estándar de direcciones I2C de 7 bits asignables a
    // dispositivos (0x00-0x07 y 0x78-0x7F están reservados por el
    // protocolo para fines especiales, por eso se excluyen).
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            found++;
            Serial.printf("  -> Dispositivo en 0x%02X : %s\n", addr, identifyDevice(addr));
        } else if (error == 4) {
            // Error 4 = "unknown error" del periférico Wire; se reporta
            // por separado de un simple NACK (error 2) porque suele
            // indicar un problema eléctrico más que ausencia del
            // dispositivo.
            Serial.printf("  -> Error desconocido en 0x%02X (posible problema electrico)\n", addr);
        }
    }

    if (found == 0) {
        Serial.println(F("  (sin dispositivos detectados en este bus/canal)"));
    }

    return found;
}

// -----------------------------------------------------------------------------
// selectMuxChannelRaw()
// -----------------------------------------------------------------------------
void selectMuxChannelRaw(uint8_t channel) {
    if (channel >= PCA9548A_CHANNEL_COUNT) {
        Serial.printf("[I2CBus] Canal %d fuera de rango (0-7), ignorado.\n", channel);
        return;
    }

    Wire.beginTransmission(PCA9548A_ADDR);
    Wire.write(static_cast<uint8_t>(1 << channel));
    Wire.endTransmission();
}

// -----------------------------------------------------------------------------
// closeAllMuxChannels()
// -----------------------------------------------------------------------------
void closeAllMuxChannels() {
    Wire.beginTransmission(PCA9548A_ADDR);
    Wire.write(static_cast<uint8_t>(0x00));
    Wire.endTransmission();
}

// -----------------------------------------------------------------------------
// scanAllMuxChannels()
// -----------------------------------------------------------------------------
void scanAllMuxChannels() {
    Serial.println(F("\n[I2CBus] === Escaneo del bus raiz (antes del mux) ==="));
    scanBus();

    for (uint8_t ch = 0; ch < PCA9548A_CHANNEL_COUNT; ch++) {
        // El Canal 0 corresponde al extensor P82B715 sobre Cat6: se baja
        // la frecuencia antes de escanear para no arriesgar falsos
        // negativos por la capacitancia adicional del cable.
        if (ch == CH_EXT_LONGBUS) {
            setFrequency(I2C_FREQ_EXTENDED_HZ);
            Serial.println(F("\n[I2CBus] === Canal 0 (P82B715 / bus extendido Cat6) ==="));
        } else {
            setFrequency(I2C_FREQ_LOCAL_HZ);
            Serial.printf("\n[I2CBus] === Canal %d (bus local) ===\n", ch);
        }

        selectMuxChannelRaw(ch);
        scanBus();
        closeAllMuxChannels();
    }

    // Se deja el bus en la frecuencia local por defecto al terminar.
    setFrequency(I2C_FREQ_LOCAL_HZ);
    Serial.println(F("\n[I2CBus] Escaneo completo de los 8 canales finalizado.\n"));
}

} // namespace I2CBus
