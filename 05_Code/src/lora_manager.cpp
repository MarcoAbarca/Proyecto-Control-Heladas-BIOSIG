/**
 * =============================================================================
 * lora_manager.cpp
 * =============================================================================
 */

#include "lora_manager.h"
#include "config.h"
#include <SPI.h>
#include <cstring>

// -----------------------------------------------------------------------------
// Flag de recepción asíncrona (patrón estándar de RadioLib: la interrupción
// de DIO1 solo puede llamar una función libre, no un método de clase). Como
// solo existe UN radio físico por nodo, un flag a nivel de archivo es una
// simplificación razonable en vez de un mecanismo de callback más genérico.
// -----------------------------------------------------------------------------
#ifdef NODE_ROLE_RECEPTOR
static volatile bool g_p2pPacketReceived = false;

static void onP2PPacketReceived() {
    g_p2pPacketReceived = true;
}
#endif

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
LoRaManager::LoRaManager()
    : _radio(new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN)) {}

// -----------------------------------------------------------------------------
// begin()
// -----------------------------------------------------------------------------
bool LoRaManager::begin() {
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, LORA_CS_PIN);

    int16_t state = _radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaManager] ERROR: radio.begin() fallo, codigo %d\n", state);
        return false;
    }

    _radioReady = true;
    Serial.println(F("[LoRaManager] SX1262 inicializado correctamente."));

#ifdef NODE_ROLE_EMISOR
    configureP2PParams();
#endif

#ifdef NODE_ROLE_RECEPTOR
    // El Receptor arranca escuchando P2P; el bridging a TTN se dispara
    // explícitamente desde la FSM de main.cpp, no automáticamente aquí.
    beginP2PListen();
#endif

    return true;
}

// -----------------------------------------------------------------------------
// configureP2PParams() [privado]
// -----------------------------------------------------------------------------
void LoRaManager::configureP2PParams() {
    _radio.setFrequency(LORA_P2P_FREQUENCY_MHZ);
    _radio.setBandwidth(LORA_P2P_BANDWIDTH_KHZ);
    _radio.setSpreadingFactor(LORA_P2P_SPREADING_FACTOR);
    _radio.setCodingRate(LORA_P2P_CODING_RATE);
    _radio.setSyncWord(LORA_P2P_SYNC_WORD);
    _radio.setOutputPower(LORA_P2P_TX_POWER_DBM);
    _radio.setPreambleLength(LORA_P2P_PREAMBLE_LENGTH);
}

// =============================================================================
// ROL EMISOR
// =============================================================================
#ifdef NODE_ROLE_EMISOR

bool LoRaManager::transmitPayload(const TelemetryPayload &payload) {
    if (!_radioReady) {
        Serial.println(F("[LoRaManager] ERROR: radio no inicializado, no se puede transmitir."));
        return false;
    }

    // RadioLib::transmit(uint8_t*, size_t) NO acepta un puntero const —
    // se copia a un buffer local en vez de castear el const away sobre
    // el propio payload (evita modificar por error el original, aunque
    // RadioLib no debería escribirlo).
    uint8_t buffer[sizeof(TelemetryPayload)];
    memcpy(buffer, &payload, sizeof(TelemetryPayload));

    int16_t state = _radio.transmit(buffer, sizeof(buffer));

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaManager] ERROR: transmit() fallo, codigo %d\n", state);
        return false;
    }

    Serial.printf("[LoRaManager] Payload transmitido (%d bytes) via P2P.\n", (int)sizeof(TelemetryPayload));
    return true;
}

#endif // NODE_ROLE_EMISOR

// =============================================================================
// ROL RECEPTOR
// =============================================================================
#ifdef NODE_ROLE_RECEPTOR

bool LoRaManager::beginP2PListen() {
    configureP2PParams();

    _radio.setDio1Action(onP2PPacketReceived);

    int16_t state = _radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaManager] ERROR: startReceive() fallo, codigo %d\n", state);
        return false;
    }

    g_p2pPacketReceived = false;
    Serial.println(F("[LoRaManager] Escucha P2P activa."));
    return true;
}

bool LoRaManager::checkReceivedP2P(TelemetryPayload &outPayload) {
    if (!g_p2pPacketReceived) {
        return false; // Nada nuevo: estado normal, no es un error.
    }

    g_p2pPacketReceived = false;

    uint8_t buffer[sizeof(TelemetryPayload)];
    int16_t state = _radio.readData(buffer, sizeof(buffer));

    // Siempre se re-arma la recepción, exista o no un payload válido:
    // de lo contrario el Receptor dejaría de escuchar tras el primer
    // paquete corrupto.
    _radio.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaManager] ERROR: readData() fallo, codigo %d\n", state);
        return false;
    }

    // El tamaño real recibido se valida indirectamente: si readData()
    // devolvió RADIOLIB_ERR_NONE con un buffer del tamaño exacto de
    // TelemetryPayload, RadioLib ya garantiza que la longitud coincide
    // (trunca/falla en vez de rellenar con basura). Aun así se copia por
    // memcpy explícito en vez de reinterpret_cast del buffer crudo, para
    // no depender de alineación de memoria del buffer temporal.
    memcpy(&outPayload, buffer, sizeof(TelemetryPayload));

    int rssi = static_cast<int>(_radio.getRSSI());
    Serial.printf("[LoRaManager] Payload P2P recibido de nodo %d (RSSI=%d dBm).\n",
                  outPayload.nodeId, rssi);
    return true;
}

bool LoRaManager::joinTTN() {
    if (_joinedTTN) {
        return true; // Ya hay sesion activa, no repetir el join.
    }

    // Reconfigura el radio a parámetros LoRaWAN/AU915 (distinto de los
    // parámetros P2P crudos usados en beginP2PListen()).
    LoRaWANNode node(&_radio, &AU915, TTN_SUBBAND);

    uint64_t joinEUI = 0, devEUI = 0;
    // TTN_JOIN_EUI/TTN_DEV_EUI en config.h están en formato de arreglo de
    // bytes (MSB primero); se combinan aquí al formato uint64_t que pide
    // LoRaWANNode.
    for (uint8_t i = 0; i < 8; i++) {
        joinEUI = (joinEUI << 8) | TTN_JOIN_EUI[i];
        devEUI  = (devEUI  << 8) | TTN_DEV_EUI[i];
    }

    int16_t state = node.beginOTAA(joinEUI, devEUI,
                                    const_cast<uint8_t*>(TTN_APP_KEY),
                                    const_cast<uint8_t*>(TTN_APP_KEY));
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaManager] ERROR: beginOTAA() fallo, codigo %d\n", state);
        beginP2PListen(); // No dejar el radio en un estado a medio configurar.
        return false;
    }

    state = node.activateOTAA();
    // RADIOLIB_LORAWAN_NEW_SESSION: join exitoso, sesion nueva.
    if (state != RADIOLIB_LORAWAN_NEW_SESSION && state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaManager] ERROR: activateOTAA() fallo, codigo %d\n", state);
        beginP2PListen();
        return false;
    }

    _joinedTTN = true;
    Serial.println(F("[LoRaManager] Join OTAA a TTN exitoso."));
    return true;
}

bool LoRaManager::bridgeToTTN(const TelemetryPayload &payload) {
    if (!joinTTN()) {
        return false; // joinTTN() ya restauro el modo P2P si fallo.
    }

    LoRaWANNode node(&_radio, &AU915, TTN_SUBBAND);

    // NOTA: node aquí es una instancia local nueva por llamada. RadioLib
    // guarda el estado de sesión OTAA (claves de sesión derivadas) del
    // lado del propio objeto LoRaWANNode, no del radio — en una
    // implementación de producción conviene persistir el objeto `node`
    // (o su sesión serializada) en vez de recrearlo en cada bridge, para
    // no tener que rehacer el join en cada ciclo. Se deja así en esta
    // fase para mantener el alcance acotado; es el primer punto a
    // optimizar si el join a TTN resulta costoso en tiempo de aire.
    // Mismo motivo que en transmitPayload(): sendReceive() de RadioLib
    // tampoco acepta un puntero const, se copia a un buffer local.
    uint8_t buffer[sizeof(TelemetryPayload)];
    memcpy(buffer, &payload, sizeof(TelemetryPayload));

    int16_t state = node.sendReceive(buffer, sizeof(buffer));

    // Se vuelve a modo P2P SIEMPRE, haya salido bien o mal el uplink:
    // el Receptor no puede quedarse sordo por una falla de TTN.
    beginP2PListen();

    if (state != RADIOLIB_ERR_NONE && state != RADIOLIB_LORAWAN_NO_DOWNLINK) {
        // RADIOLIB_LORAWAN_NO_DOWNLINK es normal: el uplink salió bien,
        // simplemente TTN no mandó downlink en la ventana RX. No es error.
        Serial.printf("[LoRaManager] ERROR: uplink a TTN fallo, codigo %d\n", state);
        return false;
    }

    Serial.println(F("[LoRaManager] Uplink a TTN completado."));
    return true;
}

bool LoRaManager::isJoinedTTN() const {
    return _joinedTTN;
}

#endif // NODE_ROLE_RECEPTOR
