/**
 * =============================================================================
 * main.cpp
 * -----------------------------------------------------------------------------
 * Integración final de la FSM, diferenciada por rol de compilación
 * (NODE_ROLE_EMISOR / NODE_ROLE_RECEPTOR, ver platformio.ini). Cada rol
 * tiene su propia máquina de estados porque hacen trabajos distintos
 * sobre hardware distinto (el Emisor lee I2C y transmite; el Receptor
 * escucha, respalda en SD y hace de puente a TTN) — mezclarlas en una
 * sola FSM genérica sería más confuso que compartir, no menos.
 *
 * Ambas FSM siguen el mismo principio no bloqueante ya usado en fases
 * anteriores: comparaciones de millis(), nunca delay() esperando trabajo.
 * =============================================================================
 */

#include <Arduino.h>
#include "config.h"
#include "telemetry.h"
#include "lora_manager.h"

// =============================================================================
// Umbrales de negocio (Sección 3 del brief): se evalúa sobre el estrato de
// copa MÁS FRÍO entre los MLX90614 que respondieron este ciclo -- la
// helada no es uniforme en la canopia, y el estrato más frío es el que
// primero entra en riesgo real de daño foliar. Si ningún MLX respondió, se
// usa el BME280 base como respaldo (menos representativo, pero mejor que
// nada).
// =============================================================================
static void evaluateAlerts(TelemetryPayload &payload) {
    bool haveTemp = false;
    float refTempC = 0.0f;

    if (payload.statusFlags & FLAG_MLX_TOP_OK) {
        refTempC = Telemetry::unpackScaled(payload.tempCanopyTop_x100, TEMP_SCALE_FACTOR);
        haveTemp = true;
    }
    if (payload.statusFlags & FLAG_MLX_MID_OK) {
        float t = Telemetry::unpackScaled(payload.tempCanopyMid_x100, TEMP_SCALE_FACTOR);
        if (!haveTemp || t < refTempC) refTempC = t;
        haveTemp = true;
    }
    if (payload.statusFlags & FLAG_MLX_LOW_OK) {
        float t = Telemetry::unpackScaled(payload.tempCanopyLow_x100, TEMP_SCALE_FACTOR);
        if (!haveTemp || t < refTempC) refTempC = t;
        haveTemp = true;
    }

    if (!haveTemp && (payload.statusFlags & FLAG_BASE_BME_OK)) {
        refTempC = Telemetry::unpackScaled(payload.tempBaseC_x100, TEMP_SCALE_FACTOR);
        haveTemp = true;
    }

    if (!haveTemp) {
        // Ningún sensor de temperatura respondió: no se puede evaluar
        // riesgo, y NO se asume "todo bien" por omisión.
        return;
    }

    if (refTempC <= FROST_THRESHOLD_C) {
        payload.statusFlags |= FLAG_ALERT_FROST;
    }
    if (refTempC > OVERHEAT_THRESHOLD_C) {
        payload.statusFlags |= FLAG_ALERT_HEAT_WINTER;
    }
}

// =============================================================================
// ROL EMISOR
// -----------------------------------------------------------------------------
// ACTUALIZADO: ya no es un loop() continuo con millis() -- es una FSM
// LINEAL. Cada wakeup por Deep Sleep reinicia el ESP32 desde cero (RAM
// normal se pierde, setup() corre una vez), así que "esperar el próximo
// ciclo" ahora significa "apagarse y dejar que el timer RTC lo reinicie",
// no un estado más del switch(). Cada "estado" del flujo pedido es una
// función que se llama una sola vez, en orden, dentro de setup().
// =============================================================================
#if defined(NODE_ROLE_EMISOR)

#include <esp_sleep.h>
#include "i2c_bus.h"
#include "pca9548a.h"
#include "sensor_manager.h"
#include "littlefs_manager.h"

// RTC_DATA_ATTR: única memoria que sobrevive al Deep Sleep (no se borra
// en el reinicio). Sin esto, msg_counter volvería a 0 en cada ciclo y el
// Receptor no podría detectar paquetes perdidos por salto en la secuencia.
RTC_DATA_ATTR static uint32_t g_sequenceNumber = 0;

// Diagnóstico persistente: cuántos ciclos SEGUIDOS terminaron sin ninguna
// lectura I2C válida. No tiene un flag propio en el payload -- ya se
// deriva de FLAG_I2C_BUS_ERROR más la ausencia de los 4 flags *_OK
// dependientes del mux -- pero se guarda igual en RTC para tener una
// racha visible por Serial/LittleFS sin tener que reconstruirla del lado
// del Receptor a partir de varios paquetes.
RTC_DATA_ATTR static uint16_t g_consecutiveEmptyReads = 0;

static PCA9548A mux(PCA9548A_ADDR);
static SensorManager sensorManager(mux);
static LittleFSManager littlefs;
static LoRaManager loraManager;

// -----------------------------------------------------------------------------
// Calcula la duración del próximo sueño con jitter, y entra en Deep Sleep.
// Es la ÚNICA forma en que este binario "vuelve" a ejecutarse: no hay
// return normal de setup() seguido de un loop() útil. Todo lo posterior
// a esta llamada nunca se alcanza (el reset lo hace el propio Deep Sleep).
// -----------------------------------------------------------------------------
static void enterDeepSleep() {
    long jitter = random(-static_cast<long>(JITTER_MAX_MS), static_cast<long>(JITTER_MAX_MS) + 1);
    long intervalMs = static_cast<long>(READ_CYCLE_INTERVAL_MS) + jitter;
    intervalMs = max(intervalMs, 1000L); // piso de 1s de seguridad

    uint64_t sleepUs = static_cast<uint64_t>(intervalMs) * 1000ULL;

    Serial.printf("[FSM] Entrando en Deep Sleep por %ld ms (ciclo #%lu completado).\n",
                  intervalMs, (unsigned long)g_sequenceNumber);
    Serial.flush(); // Asegura que el log salga por UART antes de cortar el clock.

    // Antes de dormir, todos los periféricos quedan en estado seguro:
    // mux.disableAll() ya se llama en readAllInto(); el radio SX1262 no
    // necesita apagado explícito porque el propio corte de alimentación
    // del Deep Sleep lo deja sin clock.
    esp_sleep_enable_timer_wakeup(sleepUs);
    esp_deep_sleep_start();
    // No hay código después de esta línea: el chip se reinicia al despertar.
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(SERIAL_BOOT_DELAY_MS); // Ver nota en config.h: retirar en firmware de campo final

    randomSeed(micros());

    Serial.println(F("\n===================================================="));
    Serial.println(F(" Nodo EMISOR - Wakeup por Deep Sleep"));
    Serial.printf(  " NODE_ID = %d | seq = %lu | vacios consecutivos = %u\n",
                    NODE_ID, (unsigned long)g_sequenceNumber, g_consecutiveEmptyReads);
    Serial.println(F("====================================================\n"));

    // ---------------------------------------------------------------
    // ESTADO: I2C & Sensor Init
    // ---------------------------------------------------------------
    I2CBus::begin();

    bool busOk = true;
    if (I2CBus::isBusStuck()) {
        Serial.println(F("[FSM] Bus I2C atascado al despertar. Ejecutando recoverBus()..."));
        busOk = I2CBus::recoverBus();
    }

    bool muxOk = busOk && mux.begin();
    if (!muxOk) {
        Serial.println(F("[FSM] ERROR: PCA9548A no responde tras recuperar el bus."));
    }

    // ---------------------------------------------------------------
    // ESTADO: Read Sensors (MLX90614 x3, BME280, ADC suelo/batería)
    // -----------------------------------------------------------------
    // Camino rápido si el mux no responde: NO se intentan los 4 canales
    // I2C (cada uno tardaría hasta I2C_TRANSACTION_TIMEOUT_MS en fallar,
    // ~400ms en total con el CPU despierto). Se salta directo a las
    // lecturas ADC (independientes del bus I2C) y se marca el error.
    // ---------------------------------------------------------------
    TelemetryPayload payload = Telemetry::makeEmptyPayload(NODE_ID, g_sequenceNumber);
    bool anyI2COk;

    if (muxOk) {
        anyI2COk = sensorManager.readAllInto(payload);
    } else {
        Serial.println(F("[FSM] Mux no disponible: se omiten los 4 canales I2C, salto directo a ADC."));
        payload.statusFlags |= FLAG_I2C_BUS_ERROR;
        anyI2COk = sensorManager.readSoilOnly(payload);
    }

    if (!anyI2COk) {
        g_consecutiveEmptyReads++;
        Serial.printf("[FSM] ADVERTENCIA: ningun sensor I2C respondio este ciclo (racha: %u).\n",
                      g_consecutiveEmptyReads);
    } else {
        g_consecutiveEmptyReads = 0;
    }

    evaluateAlerts(payload);
    if (payload.statusFlags & FLAG_ALERT_FROST) {
        Serial.println(F("[FSM] *** ALERTA: riesgo de helada detectado. ***"));
    }
    if (payload.statusFlags & FLAG_ALERT_HEAT_WINTER) {
        Serial.println(F("[FSM] *** ALERTA: sobrecalentamiento detectado. ***"));
    }

    // ---------------------------------------------------------------
    // ESTADO: Save to LittleFS
    // ---------------------------------------------------------------
    if (littlefs.begin()) {
        if (!littlefs.logPayload(payload)) {
            Serial.println(F("[FSM] ERROR al escribir respaldo en LittleFS (se continua igual)."));
        }
    } else {
        Serial.println(F("[FSM] ADVERTENCIA: LittleFS no disponible, sin respaldo local este ciclo."));
    }

    // ---------------------------------------------------------------
    // ESTADO: LoRa P2P Transmit
    // -----------------------------------------------------------------
    // Se intenta transmitir SIEMPRE, incluso en el escenario más
    // degradado (bus caído + LittleFS caído): un payload con
    // statusFlags=0x00 pero con nodeId/seq válidos ya le dice al
    // Receptor "este nodo esta vivo pero con fallas", que es justamente
    // la "transmisión de emergencia" pedida -- no hay un modo separado,
    // es el mismo transmitPayload() de siempre aplicado al mismo payload
    // degradado.
    // ---------------------------------------------------------------
    if (loraManager.begin()) {
        if (!loraManager.transmitPayload(payload)) {
            Serial.println(F("[FSM] ERROR al transmitir por LoRa este ciclo."));
        }
    } else {
        Serial.println(F("[FSM] ERROR: radio LoRa no disponible, ciclo sin transmision."));
    }

    g_sequenceNumber++;

    // ---------------------------------------------------------------
    // ESTADO: Prepare & Enter Deep Sleep
    // ---------------------------------------------------------------
    enterDeepSleep(); // No retorna.
}

void loop() {
    // Nunca se alcanza: enterDeepSleep() al final de setup() reinicia el
    // chip antes de que el framework Arduino llegue a llamar a loop().
    // Se deja vacío porque el framework exige que la función exista.
}

#endif // NODE_ROLE_EMISOR

// =============================================================================
// ROL RECEPTOR
// =============================================================================
#if defined(NODE_ROLE_RECEPTOR)

#include "sd_manager.h"

enum class ReceptorState {
    INIT,
    LISTEN_P2P,
    BRIDGE_TTN
};

static LoRaManager loraManager;
static SDManager sdManager;

static ReceptorState currentState = ReceptorState::INIT;
static unsigned long lastUplinkTime = 0;

// Solo se conserva el ULTIMO payload recibido entre uplinks: LoRaWAN no es
// apto para reenviar cada paquete P2P individualmente (satura el duty
// cycle regulatorio), así que el uplink a TTN es un muestreo periódico,
// no un espejo 1:1 de lo recibido por P2P. El respaldo COMPLETO de cada
// paquete (sin muestreo) queda en la MicroSD via sd_manager.
static TelemetryPayload pendingPayload;
static bool hasPendingPayload = false;

static uint32_t packetsReceivedCount = 0;
static uint32_t packetsLoggedCount = 0;
static unsigned long lastStatusReport = 0;

static const __FlashStringHelper* receptorStateName(ReceptorState state) {
    switch (state) {
        case ReceptorState::INIT: return F("INIT");
        case ReceptorState::LISTEN_P2P: return F("LISTEN_P2P");
        case ReceptorState::BRIDGE_TTN: return F("BRIDGE_TTN");
        default: return F("UNKNOWN");
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(SERIAL_BOOT_DELAY_MS);

    Serial.println(F("\n===================================================="));
    Serial.println(F(" Nodo RECEPTOR - Gateway P2P + SD + Puente TTN"));
    Serial.println(F("====================================================\n"));

    currentState = ReceptorState::INIT;
}

void loop() {
    static ReceptorState previousState = ReceptorState::INIT;
    if (currentState != previousState) {
        Serial.printf("[FSM] Estado: %s -> %s\n",
                      receptorStateName(previousState),
                      receptorStateName(currentState));
        previousState = currentState;
    }

    switch (currentState) {

        // ---------------------------------------------------------------
        case ReceptorState::INIT: {
            Serial.println(F("[FSM] INIT: inicializando radio LoRa..."));
            if (!loraManager.begin()) {
                Serial.println(F("[FSM] ERROR: radio LoRa no disponible. Reintentando..."));
                break; // Reintento no bloqueante en la proxima vuelta de loop().
            }

            // La SD es respaldo, no crítica para la función de puente: si
            // falla el montaje, el Receptor sigue operando (recibe P2P y
            // sube a TTN) pero sin backup local, y lo deja bien logueado.
            if (!sdManager.begin()) {
                Serial.println(F("[FSM] ADVERTENCIA: MicroSD no disponible, se continua sin respaldo local."));
            } else {
                Serial.println(F("[FSM] MicroSD lista para respaldo."));
            }

            Serial.printf("[FSM] Inicializacion completa. Escuchando P2P. Uplink cada %lu ms.\n",
                          static_cast<unsigned long>(TTN_UPLINK_INTERVAL_MS));
            lastUplinkTime = millis();
            lastStatusReport = millis();
            currentState = ReceptorState::LISTEN_P2P;
            break;
        }

        // ---------------------------------------------------------------
        case ReceptorState::LISTEN_P2P: {
            TelemetryPayload incoming;

            if (loraManager.checkReceivedP2P(incoming)) {
                packetsReceivedCount++;
                Serial.printf("[FSM] Paquete P2P #%lu recibido (nodo=%d, seq=%lu).\n",
                              (unsigned long)packetsReceivedCount, incoming.nodeId,
                              (unsigned long)incoming.sequenceNumber);
                Serial.printf("[FSM] Estado payload: flags=0x%04X, SD=%s.\n",
                              incoming.statusFlags,
                              sdManager.isReady() ? "disponible" : "no disponible");

                if (incoming.statusFlags & FLAG_ALERT_FROST) {
                    Serial.println(F("[FSM] *** El nodo emisor reporto riesgo de helada. ***"));
                }

                if (sdManager.isReady()) {
                    if (sdManager.logPayload(incoming)) {
                        packetsLoggedCount++;
                        Serial.printf("[FSM] Payload guardado en SD (total=%lu).\n",
                                      static_cast<unsigned long>(packetsLoggedCount));
                    } else {
                        Serial.println(F("[FSM] ERROR al escribir en SD, el dato solo llegara via TTN."));
                    }
                }

                pendingPayload = incoming;
                hasPendingPayload = true;
            }

            if (millis() - lastStatusReport >= 10000UL) {
                Serial.printf("[FSM] LISTEN_P2P activo: recibidos=%lu, guardados=%lu, pendiente=%s, proximo uplink en %lu ms.\n",
                              static_cast<unsigned long>(packetsReceivedCount),
                              static_cast<unsigned long>(packetsLoggedCount),
                              hasPendingPayload ? "si" : "no",
                              static_cast<unsigned long>(TTN_UPLINK_INTERVAL_MS) -
                                  min(static_cast<unsigned long>(millis() - lastUplinkTime),
                                      static_cast<unsigned long>(TTN_UPLINK_INTERVAL_MS)));
                lastStatusReport = millis();
            }

            // Ventana de tiempo cumplida: se interrumpe la escucha P2P
            // para hacer el puente hacia TTN (ver time-multiplexing en
            // lora_manager.h). No se hace por cada paquete individual.
            if (millis() - lastUplinkTime >= TTN_UPLINK_INTERVAL_MS) {
                Serial.println(F("[FSM] Ventana TTN cumplida: cambiando a BRIDGE_TTN."));
                currentState = ReceptorState::BRIDGE_TTN;
            }
            break;
        }

        // ---------------------------------------------------------------
        case ReceptorState::BRIDGE_TTN: {
            bool p2pReady = true;
            Serial.printf("[FSM] BRIDGE_TTN: payload pendiente=%s.\n",
                          hasPendingPayload ? "si" : "no");

            if (hasPendingPayload) {
                Serial.println(F("[FSM] Iniciando puente hacia TTN (radio deja de escuchar P2P)..."));

                if (loraManager.bridgeToTTN(pendingPayload)) {
                    Serial.println(F("[FSM] Uplink a TTN exitoso."));
                } else {
                    Serial.println(F("[FSM] Uplink a TTN fallido (el dato ya quedo respaldado en SD)."));
                    // bridgeToTTN() ya intenta restaurar P2P. Este segundo
                    // intento evita declarar LISTEN_P2P si esa restauracion
                    // fallo por un estado transitorio del radio.
                    p2pReady = loraManager.beginP2PListen();
                }
            } else {
                Serial.println(F("[FSM] Sin datos nuevos desde el ultimo uplink, se omite este ciclo TTN."));
            }

            if (!p2pReady) {
                Serial.println(F("[FSM] ERROR: escucha P2P no restaurada. Volviendo a INIT para reintentar el radio."));
                currentState = ReceptorState::INIT;
                break;
            }

            hasPendingPayload = false;
            lastUplinkTime = millis();
            Serial.println(F("[FSM] Radio P2P restaurada. Volviendo a LISTEN_P2P."));
            currentState = ReceptorState::LISTEN_P2P;
            break;
        }
    }
}

#endif // NODE_ROLE_RECEPTOR

// =============================================================================
// Guardia de compilación: evita un binario "sin rol" silencioso si alguien
// compila el entorno base [env] directamente en vez de nodo_emisor/nodo_receptor.
// =============================================================================
#if !defined(NODE_ROLE_EMISOR) && !defined(NODE_ROLE_RECEPTOR)
#error "Definir NODE_ROLE_EMISOR o NODE_ROLE_RECEPTOR (compilar con -e nodo_emisor o -e nodo_receptor)"
#endif
