/**
 * =============================================================================
 * lora_manager.h
 * -----------------------------------------------------------------------------
 * Capa de radio sobre RadioLib (SX1262), diferenciada por rol mediante
 * #ifdef NODE_ROLE_EMISOR / NODE_ROLE_RECEPTOR (config de platformio.ini).
 *
 * DECISIÓN DE DISEÑO IMPORTANTE (confirmar con Gemini/validar en campo):
 * El Receptor tiene un ÚNICO radio físico que debe cumplir dos roles que
 * NO pueden coexistir con la misma configuración de modem al mismo tiempo:
 *   1. Escuchar continuamente los paquetes P2P de los nodos Emisores
 *      (parámetros "crudos": SF/BW/syncWord privados, ver config.h).
 *   2. Unirse y transmitir a TTN vía LoRaWAN (parámetros de la región
 *      AU915 gestionados internamente por RadioLib's LoRaWANNode).
 *
 * Por eso el Receptor TIME-MULTIPLEXA el radio: escucha en modo P2P la
 * mayor parte del tiempo, y periódicamente reconfigura el mismo radio a
 * modo LoRaWAN para hacer el uplink a TTN, y vuelve a P2P al terminar.
 * Durante esa ventana de bridging (unos segundos), el Receptor NO puede
 * recibir paquetes P2P — es una limitación física de tener un solo radio,
 * no un bug. Si esto no es aceptable operativamente, la alternativa es un
 * segundo módulo de radio dedicado solo a LoRaWAN.
 *
 * VERIFICAR: la API de LoRaWAN de RadioLib ha tenido cambios entre
 * versiones (ver jgromes/RadioLib#1806). El código de esta fase está
 * escrito contra el patrón estable documentado en los ejemplos oficiales;
 * confirmar contra la versión exacta fijada en platformio.ini al compilar.
 * =============================================================================
 */

#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <Arduino.h>
#include <RadioLib.h> // Incluye LoRaWANNode y las bandas regionales (AU915, etc.)
#include "telemetry.h"

class LoRaManager {
public:
    LoRaManager();

    /**
     * Inicializa el bus SPI (pines de config.h), el módulo SX1262 y lo
     * deja configurado en modo P2P (parámetros LORA_P2P_* de config.h).
     * Debe llamarse una sola vez en setup(), antes de cualquier otra
     * operación de radio.
     */
    bool begin();

#ifdef NODE_ROLE_EMISOR
    /**
     * Transmite un TelemetryPayload por LoRa P2P. Es una llamada
     * BLOQUEANTE (RadioLib::transmit()), pero acotada: a SF9/125kHz un
     * payload de 30 bytes tarda del orden de 100-200ms en el aire, un
     * tiempo aceptable como paso discreto de la FSM (estado TRANSMIT),
     * no una espera indefinida.
     *
     * @return true si la transmisión se completó sin error de radio.
     */
    bool transmitPayload(const TelemetryPayload &payload);
#endif

#ifdef NODE_ROLE_RECEPTOR
    /**
     * (Re)configura el radio en modo P2P y arma la recepción continua
     * en segundo plano (startReceive() + interrupción en DIO1). Debe
     * llamarse en begin() y también después de volver de
     * bridgeToTTN(), ya que esa operación reconfigura el modem.
     */
    bool beginP2PListen();

    /**
     * Chequeo NO BLOQUEANTE: si llegó un paquete P2P desde que se armó
     * la escucha, lo copia en `outPayload`, valida su tamaño (debe
     * coincidir con sizeof(TelemetryPayload); un tamaño distinto indica
     * ruido/colisión, no un payload válido) y vuelve a armar la
     * recepción para el próximo paquete.
     *
     * @return true si se recibió y validó un payload nuevo en esta
     *         llamada. false en cualquier otro caso (incluyendo "todavía
     *         no llegó nada", que no es un error).
     */
    bool checkReceivedP2P(TelemetryPayload &outPayload);

    /**
     * Se une a TTN por OTAA (solo la primera vez; si ya hay sesión
     * activa, no repite el join). Reconfigura el radio a parámetros
     * LoRaWAN/AU915 — ver nota de time-multiplexing arriba.
     */
    bool joinTTN();

    /**
     * Reconfigura el radio a modo LoRaWAN, envía `payload` a TTN como
     * uplink, y al finalizar SIEMPRE vuelve a modo P2P (llama
     * internamente a beginP2PListen()) para no dejar al Receptor sordo
     * de forma permanente ante un error de la etapa TTN.
     *
     * @return true si el uplink se transmitió sin error de radio.
     *         (No implica que TTN lo haya recibido: LoRaWAN es
     *         no-confirmado por defecto en esta implementación.)
     */
    bool bridgeToTTN(const TelemetryPayload &payload);

    bool isJoinedTTN() const;
#endif

private:
    SX1262 _radio;
    bool _radioReady = false;

#ifdef NODE_ROLE_RECEPTOR
    // La sesión LoRaWAN y sus contadores deben sobrevivir entre uplinks.
    LoRaWANNode *_ttnNode = nullptr;
    bool _joinedTTN = false;
#endif

    void configureP2PParams();
};

#endif // LORA_MANAGER_H
