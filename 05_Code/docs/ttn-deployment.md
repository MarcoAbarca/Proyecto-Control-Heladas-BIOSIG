# Preparacion de TTN y gateways

## Archivos que se suben a TTN

### Payload Formatter

Subir el contenido de:

`04_Panel_Nube/payload_formatter.js`

En TTN Console:

1. Abrir la aplicacion.
2. Entrar a `Payload formatters`.
3. Seleccionar `Uplink`.
4. Seleccionar formatter `JavaScript`.
5. Pegar el contenido de `payload_formatter.js`.
6. Guardar y probar con un payload hexadecimal de 23 bytes.

El contrato de offsets y escalas esta documentado en:

`docs/telemetry-contract.md`

### Credenciales del dispositivo

Todavia no se debe subir la configuracion actual de `include/config.h`: las
credenciales son valores de prueba en cero. Cuando creemos el dispositivo en
TTN, las credenciales OTAA reales se colocaran solo en un archivo local o en
variables seguras de PlatformIO, nunca en el repositorio.

Se necesitaran:

- `DevEUI` del dispositivo.
- `JoinEUI` de la aplicacion.
- `AppKey` del dispositivo.
- Plan de frecuencias exacto de la region.
- Sub-banda, si el plan de frecuencias la requiere.

## Curico y Rengo

Una antena pasiva no se registra como antena independiente en TTN. Lo que se
registra es un gateway LoRaWAN con su concentrador, identificador, plan de
frecuencias y ubicacion. Para cada punto necesitamos:

- `Gateway ID`.
- Fabricante y modelo del gateway/concentrador.
- Plan de frecuencias y sub-banda.
- Ubicacion aproximada y altura.
- Tipo de conexion IP del gateway, si aplica.
- Estado de conexion y ultima actividad en TTN.

Si Curico y Rengo son dos gateways TTN independientes, el mismo dispositivo
LoRaWAN puede ser escuchado por ambos. No se selecciona una antena desde el
dispositivo: cada gateway recibe el uplink y TTN elimina duplicados. Esto
mejora la cobertura y la redundancia si ambos tienen el mismo plan de
frecuencias.

## Varias antenas en un mismo radio

No se deben unir dos antenas con un divisor pasivo improvisado al mismo
SX1262. Puede producir desadaptacion, perdida de potencia o dano en la etapa
RF. Para usar diversidad o varias antenas en un solo equipo se necesita
hardware RF disenado para ello, como un switch de antena controlado o un
gateway con entradas y concentradores compatibles.

## Open Five Server en VS Code

Open Five Server es la extension que usaremos para servir y abrir el dashboard
web durante el desarrollo. No forma parte del enlace LoRaWAN ni de la
conexion de los gateways a TTN.

La ruta de datos del sistema sigue siendo:

`Gateway LoRaWAN -> TTN -> backend PostgreSQL -> dashboard web`

Cuando el dashboard este implementado, lo levantaremos desde VS Code con esa
extension para revisar la interfaz en el navegador. Para produccion podremos
usar despues un servidor web o contenedor, sin cambiar el flujo de datos.

## Prueba de cobertura

Antes de decidir si el receptor P2P actual es suficiente, registrar por cada
uplink: `sequenceNumber`, gateway que lo recibio, RSSI, SNR, timestamp de TTN
y si el paquete aparecio duplicado en Curico y Rengo. La prueba debe cubrir
la ventana en que el receptor deja de escuchar P2P para transmitir a TTN.