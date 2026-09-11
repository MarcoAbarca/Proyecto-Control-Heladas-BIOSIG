# Documento global de traspaso

Fecha: 2026-09-07  
Proyecto: Control de Heladas BIOSIG  
Workspace: `Proyecto control heladas Biosig/05_Code`

Este documento resume el estado del proyecto para el usuario, Claude y
Gemini. Debe leerse antes de continuar cambios en el repositorio.

## 1. Objetivo

Construir un sistema de monitoreo de heladas de extremo a extremo:

`Sensores -> nodo ESP32-S3 -> LoRa P2P -> receptor -> LoRaWAN/TTN -> backend -> PostgreSQL -> dashboard web -> Telegram`

El foco actual es observabilidad: recibir telemetria, separar datos por nodo,
mostrar sensores e historicos, y configurar alertas personalizadas.

## 2. Estado actual en una frase

El contrato binario, decoder, ingesta TTN, esquema PostgreSQL, API y dashboard
local ya estan implementados y probados de forma aislada. La conexion real a
PostgreSQL y TTN aun no esta operativa porque `backend/.env` sigue usando el
placeholder `HOST` o debe completarse con los valores reales.

## 3. Arquitectura actual

### Firmware

- Placa: Seeed XIAO ESP32-S3.
- Radio: SX1262 / Wio-SX1262.
- Roles PlatformIO:
  - `nodo_emisor`: sensores, LittleFS, P2P y deep sleep.
  - `nodo_receptor`: escucha P2P, MicroSD y puente LoRaWAN hacia TTN.
- Prueba de enlace: `lora_test` con entornos `lora_tx` y `lora_rx`.
- El receptor usa un solo radio para escuchar P2P y, periódicamente, cambiar
  a LoRaWAN para transmitir a TTN. Durante ese cambio existe una ventana ciega.
- El receptor conserva localmente los paquetes en SD, pero el firmware actual
  conserva solo el ultimo payload pendiente para el uplink TTN.

### Backend y web

- Runtime: Node.js con ES modules.
- Base: PostgreSQL administrado usando SSL.
- API HTTP y servidor de archivos: `backend/src/server.js`.
- Ingesta TTN: `POST /api/ttn/uplink`.
- Dashboard local: `backend/public/`.
- URL local: `http://localhost:8080/`.
- Open Five Server es una extension de VS Code para servir archivos web; no es
  TTN, no es Open5GS y no forma parte del transporte LoRaWAN.

## 4. Contrato de telemetria

El payload tiene exactamente 23 bytes y usa little-endian para enteros de mas
de un byte:

| Offset | Campo | Tipo | Escala |
| ---: | --- | --- | ---: |
| 0 | `nodeId` | uint8 | 1 |
| 1 | `sequenceNumber` | uint32 | 1 |
| 5 | `tempCanopyTop_x100` | int16 | 100 |
| 7 | `tempCanopyMid_x100` | int16 | 100 |
| 9 | `tempCanopyLow_x100` | int16 | 100 |
| 11 | `tempBaseC_x100` | int16 | 100 |
| 13 | `humBasePct_x100` | uint16 | 100 |
| 15 | `pressureHpa_x10` | uint16 | 10 |
| 17 | `soilMoisturePct_x100` | uint16 | 100 |
| 19 | `tempSoilC_x100` | int16 | 100 |
| 21 | `statusFlags` | uint16 | 1 |

Flags principales:

- `0x0001`: MLX superior correcto.
- `0x0002`: MLX medio correcto.
- `0x0004`: MLX inferior correcto.
- `0x0008`: BME280 correcto.
- `0x0010`: humedad de suelo correcta.
- `0x0020`: DS18B20 correcto.
- `0x0040`: error I2C.
- `0x0100`: alerta de helada.
- `0x0200`: alerta de temperatura alta.

El payload no tiene timestamp de medicion. Se usa el timestamp de TTN o de
recepcion del backend.

Fuente detallada: `docs/telemetry-contract.md` y `include/telemetry.h`.

## 5. Modelo multi-nodo

La tabla `nodes` usa `node_id` como identificador dedicado, entre 0 y 255.
Cada medicion referencia ese nodo y tiene `sequence_number`.

Restriccion de deduplicacion:

`UNIQUE (node_id, sequence_number)`

Cuando llega el primer uplink de un nodo nuevo, la ingesta crea
automaticamente:

`node_id = X, display_name = Nodo X`

El dashboard lista todos los nodos y permite seleccionar uno. Esto permite
agregar nodos sin modificar el frontend ni crear tablas nuevas.

## 6. Dashboard implementado

El dashboard muestra:

- Estado global del campo.
- Cantidad de nodos operativos y sin comunicacion.
- Temperatura minima del historico seleccionado.
- Ultima recepcion y secuencia.
- Perdidas detectadas por saltos de secuencia.
- Historico de temperatura base.
- Lista de nodos con `#nodeId`.
- Selector de nodo y rango historico.
- Ficha completa de sensores del nodo seleccionado:
  - MLX superior, medio e inferior.
  - BME280: temperatura, humedad y presion.
  - Humedad de suelo.
  - DS18B20.
  - unidad, valor, estado valido/no valido, timestamp, secuencia y flags.
- Alertas recientes.
- Configuracion de alertas por nodo:
  - umbral y activacion de helada;
  - umbral y activacion de calor;
  - sensor defectuoso;
  - perdida de conectividad;
  - habilitacion de Telegram.

Importante: la pantalla de configuracion ya existe, pero la evaluacion
automatica de reglas y el envio real de Telegram aun no estan implementados.

## 7. Base de datos

Migraciones:

- `backend/db/migrations/001_initial.sql`
  - `nodes`;
  - `telemetry_measurements`;
  - `node_current_state`;
  - `alert_events`.
- `backend/db/migrations/002_alert_settings.sql`
  - `node_alert_settings` por nodo.

La migracion se ejecuta con:

```powershell
cd backend
npm.cmd run migrate
```

Estado conocido al preparar este documento:

```text
Error: getaddrinfo ENOTFOUND HOST
```

Esto significa que `backend/.env` todavia contiene el literal `HOST` o no
contiene el servidor real. No se han insertado datos de demostracion porque la
migracion no ha terminado correctamente.

## 8. Configuracion local

Crear o editar `backend/.env`:

```powershell
cd backend
Copy-Item .env.example .env
notepad .env
```

Formato de conexion:

```text
DATABASE_URL=postgresql://USUARIO:CONTRASENA@SERVIDOR:25060/sensagri?sslmode=require
```

La contraseña no lleva comillas. Si tiene caracteres especiales de URL, debe
codificarse, por ejemplo `@` como `%40` y `#` como `%23`.

Nunca copiar `backend/.env` a commits ni pegar secretos en chats. El archivo
esta excluido por `.gitignore`.

Variables adicionales:

```text
PORT=8080
TELEMETRY_ALLOWED_NODE_IDS=
TELEGRAM_BOT_TOKEN=
TELEGRAM_CHAT_ID=
```

## 9. Arranque y pruebas

Desde `05_Code/backend`:

```powershell
npm.cmd install
npm.cmd test
npm.cmd run migrate
npm.cmd start
```

Abrir:

```text
http://localhost:8080/
```

No ejecutar dos veces `npm.cmd start` en el mismo puerto. Si aparece:

`EADDRINUSE: address already in use :::8080`

ya existe un servidor activo. Usar la URL anterior o detener el proceso antes
de arrancar otro.

Validacion alcanzada:

- 6 pruebas Node correctas.
- Sintaxis de backend y frontend correcta.
- Dashboard servido con HTTP 200.
- HTML, CSS y JavaScript verificados localmente.
- Vector del payload validado en decoder y formatter TTN.

## 10. TTN

Archivo para subir en TTN Console:

`04_Panel_Nube/payload_formatter.js`

Ruta en TTN:

`Application -> Payload formatters -> Uplink -> JavaScript`

El formatter exige exactamente 23 bytes.

Para OTAA se necesitan:

- `Application ID`;
- `Device ID`;
- `DevEUI`;
- `JoinEUI`;
- `AppKey`.

El firmware espera esos valores en `include/config.h`:

```cpp
TTN_DEV_EUI[8]
TTN_JOIN_EUI[8]
TTN_APP_KEY[16]
```

Actualmente estan en cero y no deben flashearse como configuracion real.

El firmware tambien tiene:

```cpp
#define TTN_REGION_AU915
TTN_SUBBAND = 2
```

Esto debe confirmarse contra el plan regional real de los gateways de Curico y
Rengo antes de probar OTAA.

El backend espera TTN en:

`POST https://HOST_PUBLICO/api/ttn/uplink`

`localhost` no puede recibir webhooks desde TTN. Hace falta un backend
publicado con HTTPS o usar MQTT TTN con credenciales de lectura.

## 11. Gateways, Curico y Rengo

Dos gateways LoRaWAN independientes pueden recibir el mismo nodo. TTN puede
eliminar duplicados y entregar metadatos de ambos gateways.

Para cada ubicacion falta confirmar:

- Gateway ID.
- Modelo y concentrador.
- Plan de frecuencias.
- Sub-banda.
- Ubicacion y altura.
- Tipo de conexion IP.

No conectar dos antenas pasivas directamente al mismo SX1262 mediante un
divisor improvisado. Para diversidad se necesita hardware RF apropiado.

## 12. Diferencias entre estado actual y produccion

Ya esta implementado:

- decoder y contrato;
- API de consulta;
- webhook parser TTN;
- deduplicacion;
- estado multi-nodo;
- dashboard;
- configuracion persistida de umbrales;
- migraciones.

Falta implementar:

1. Conexion exitosa a PostgreSQL real.
2. Carga de datos de prueba no destructiva.
3. Evaluador que convierta mediciones en `alert_events` activos/resueltos.
4. Envio real de Telegram con control de duplicados y frecuencia.
5. Autenticacion o restriccion de red para produccion.
6. HTTPS publico para webhook TTN.
7. Credenciales OTAA reales y validacion de region/sub-banda.
8. Prueba de campo y medicion de perdida durante la ventana TTN.
9. Mejorar la cola del receptor para no conservar solo el ultimo payload.
10. Compilar y validar todos los entornos PlatformIO en hardware real.

## 13. Instrucciones para el siguiente agente

1. No volver a crear el dashboard desde cero.
2. Leer este archivo y luego `docs/ACTUALIZACION_PROYECTO_2026-09-07.md`.
3. Verificar primero `backend/.env` sin imprimirlo.
4. Ejecutar `npm.cmd run migrate` y capturar solo el resultado, nunca secretos.
5. Si la migracion pasa, insertar datos de demostracion etiquetados y no
   destruir filas existentes.
6. Probar `/api/health`, `/api/nodes` y el dashboard.
7. Mantener el contrato de 23 bytes sincronizado entre `telemetry.h`,
   `payload_formatter.js`, `telemetry.js` y este documento.
8. Antes de modificar firmware OTAA, confirmar plan regional y sub-banda.

## 14. Archivos clave

- Firmware: `include/config.h`, `include/telemetry.h`, `src/main.cpp`,
  `src/lora_manager.cpp`, `src/sensor_manager.cpp`.
- Radio de prueba: `lora_test/src/main.cpp`.
- TTN: `../04_Panel_Nube/payload_formatter.js`.
- Contrato: `docs/telemetry-contract.md`.
- TTN y gateways: `docs/ttn-deployment.md`.
- Backend: `backend/src/server.js`, `backend/src/ingest.js`,
  `backend/src/telemetry.js`, `backend/src/migrate.js`.
- Base: `backend/db/migrations/`.
- Dashboard: `backend/public/`.
- Resumen previo: `docs/ACTUALIZACION_PROYECTO_2026-09-07.md`.

## 15. Regla de seguridad

No incluir en documentos, commits ni mensajes:

- contraseña de PostgreSQL;
- `AppKey` de TTN;
- API keys TTN;
- token de Telegram.

Usar variables de entorno, archivos locales excluidos o el administrador de
secretos del despliegue.