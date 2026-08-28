/**
 * =============================================================================
 * main.cpp
 * -----------------------------------------------------------------------------
 * Bucle principal basado en una máquina de estados finitos (FSM) no
 * bloqueante. No usa delay() para esperar el próximo ciclo de lectura:
 * el tiempo se controla comparando millis() contra READ_CYCLE_INTERVAL_MS
 * (config.h), por lo que loop() se ejecuta continuamente sin detener al
 * procesador. Sin modos de sleep en esta fase (decisión explícita del
 * proyecto) — se deja el punto de extensión marcado para cuando se
 * integre Deep/Light Sleep más adelante.
 *
 * Esta fase NO incluye drivers de sensores reales: SensorManager
 * orquesta la selección de canal y el patrón de timeout, pero cada
 * lectura reporta SensorStatus::NOT_IMPLEMENTED hasta que se asignen
 * sensores concretos a los Canales 1-7.
 * =============================================================================
 */

#include <Arduino.h>
#include "config.h"
#include "i2c_bus.h"
#include "pca9548a.h"
#include "sensor_manager.h"

// -----------------------------------------------------------------------------
// Estados de la FSM
// -----------------------------------------------------------------------------
enum class SystemState {
    INIT,             // Inicialización de bus, multiplexor y periféricos.
    WAIT_CYCLE,       // Espera no bloqueante hasta el próximo ciclo de lectura.
    SCAN_LOCAL,       // Recorre canales locales 1-7, uno por cada paso de loop().
    SCAN_EXTENDED,    // Lee el Canal 0 (extensor P82B715 / Cat6).
    AGGREGATE,        // Punto de extensión: consolidar/transmitir el ciclo leído.
    BUS_RECOVERY      // Se entra aquí si INIT o una lectura detecta bus atascado.
};

// -----------------------------------------------------------------------------
// Objetos y estado global del módulo main
// -----------------------------------------------------------------------------
static PCA9548A mux(PCA9548A_ADDR);
static SensorManager sensorManager(mux);

static SystemState currentState = SystemState::INIT;
static unsigned long lastCycleTime = 0;
static uint8_t currentLocalChannel = CH_LOCAL_1;

// -----------------------------------------------------------------------------
// Utilidad de logging de una lectura (temporal, hasta definir formato de
// transmisión LoRa/telemetría final).
// -----------------------------------------------------------------------------
static void logReading(const SensorReading &r) {
    const char* statusStr = "?";
    switch (r.status) {
        case SensorStatus::OK:              statusStr = "OK"; break;
        case SensorStatus::TIMEOUT:         statusStr = "TIMEOUT"; break;
        case SensorStatus::NO_ACK:          statusStr = "NO_ACK"; break;
        case SensorStatus::MUX_ERROR:       statusStr = "MUX_ERROR"; break;
        case SensorStatus::NOT_IMPLEMENTED: statusStr = "NOT_IMPLEMENTED"; break;
    }
    Serial.printf("  [Canal %d] status=%s t=%lums\n", r.channel, statusStr, r.timestampMs);
}

// -----------------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(SERIAL_BOOT_DELAY_MS); // Ver nota en config.h: retirar en firmware de campo

    Serial.println(F("\n===================================================="));
    Serial.println(F(" Sistema de Monitoreo Modular ESP32-S3 - FSM principal"));
    Serial.println(F("====================================================\n"));

    // La inicialización real de hardware ocurre en el primer paso de la
    // FSM (estado INIT), no aquí, para que cualquier falla de arranque
    // (ej. multiplexor no detectado) se maneje con el mismo mecanismo de
    // recuperación que una falla en pleno funcionamiento, en vez de
    // requerir lógica de arranque duplicada.
    currentState = SystemState::INIT;
}

// -----------------------------------------------------------------------------
// loop()
// -----------------------------------------------------------------------------
void loop() {
    switch (currentState) {

        // ---------------------------------------------------------------
        case SystemState::INIT: {
            I2CBus::begin();

            if (I2CBus::isBusStuck()) {
                Serial.println(F("[FSM] Bus atascado detectado en INIT."));
                currentState = SystemState::BUS_RECOVERY;
                break;
            }

            if (!mux.begin()) {
                Serial.println(F("[FSM] PCA9548A no responde. Intentando recuperar bus..."));
                currentState = SystemState::BUS_RECOVERY;
                break;
            }

            Serial.println(F("[FSM] Inicializacion completa. Entrando en ciclo de lectura.\n"));
            lastCycleTime = millis();
            currentState = SystemState::WAIT_CYCLE;
            break;
        }

        // ---------------------------------------------------------------
        case SystemState::WAIT_CYCLE: {
            // Espera no bloqueante: loop() sigue corriendo (y podría
            // atender otras tareas cooperativas futuras) mientras no se
            // cumple el intervalo configurado.
            if (millis() - lastCycleTime >= READ_CYCLE_INTERVAL_MS) {
                Serial.println(F("[FSM] --- Nuevo ciclo de lectura ---"));
                currentLocalChannel = CH_LOCAL_1;
                currentState = SystemState::SCAN_LOCAL;
            }
            break;
        }

        // ---------------------------------------------------------------
        case SystemState::SCAN_LOCAL: {
            if (currentLocalChannel <= CH_LOCAL_7) {
                SensorReading r = sensorManager.readChannel(currentLocalChannel);
                logReading(r);

                if (r.status == SensorStatus::MUX_ERROR) {
                    // Un error de multiplexor en pleno ciclo es más grave
                    // que un sensor individual sin implementar: se aborta
                    // el ciclo y se intenta recuperar el bus.
                    currentState = SystemState::BUS_RECOVERY;
                    break;
                }

                currentLocalChannel++;
                // Se permanece en SCAN_LOCAL: el próximo canal se procesa
                // en la siguiente vuelta de loop(), no en la misma, para
                // no acumular todas las lecturas del ciclo en un único
                // paso bloqueante.
            } else {
                currentState = SystemState::SCAN_EXTENDED;
            }
            break;
        }

        // ---------------------------------------------------------------
        case SystemState::SCAN_EXTENDED: {
            SensorReading r = sensorManager.readExtendedChannel();
            logReading(r);

            if (r.status == SensorStatus::MUX_ERROR) {
                currentState = SystemState::BUS_RECOVERY;
                break;
            }

            currentState = SystemState::AGGREGATE;
            break;
        }

        // ---------------------------------------------------------------
        case SystemState::AGGREGATE: {
            // Punto de extensión: aquí se consolidarán las N lecturas del
            // ciclo (buffer/estructura de datos) para transmisión LoRa o
            // almacenamiento, una vez definido ese formato. También es el
            // lugar natural donde, más adelante, se decidiría entrar a
            // Light/Deep Sleep antes de volver a WAIT_CYCLE.
            Serial.println(F("[FSM] Ciclo de lectura finalizado.\n"));

            mux.disableAll(); // Estado seguro por defecto entre ciclos.
            lastCycleTime = millis();
            currentState = SystemState::WAIT_CYCLE;
            break;
        }

        // ---------------------------------------------------------------
        case SystemState::BUS_RECOVERY: {
            if (I2CBus::recoverBus() && mux.begin()) {
                Serial.println(F("[FSM] Bus recuperado. Reintentando inicializacion.\n"));
                currentState = SystemState::INIT;
            } else {
                // No se usa delay(): se reintenta en vueltas sucesivas de
                // loop(), espaciadas por el propio tiempo de ejecución de
                // recoverBus(). Si esto persistiera muchos ciclos, sería
                // señal de una falla de hardware real (ver ADVERTENCIA
                // impresa por I2CBus::recoverBus()).
                Serial.println(F("[FSM] Recuperacion fallida, se reintentara."));
            }
            break;
        }
    }
}
