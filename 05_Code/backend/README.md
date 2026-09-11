# Backend de Control de Heladas

Esta carpeta contiene el contrato y la futura ingesta TTN del sistema.

## Dashboard local

El backend sirve el dashboard desde `public/`. Con `DATABASE_URL` configurada:

```text
npm.cmd start
```

Abrir `http://localhost:8080/` en el navegador. Open Five Server tambien puede
servir los archivos de `public/` para revisar la interfaz mientras se trabaja
en el frontend, aunque los datos reales los entrega la API del backend.

## Configuracion

Copiar `.env.example` a un archivo local de variables de entorno y completar
los valores fuera del repositorio. No poner credenciales en el codigo ni en
los archivos de migracion.

```powershell
Copy-Item .env.example .env
# Editar .env y completar DATABASE_URL localmente
npm.cmd run migrate
```

Los comandos `start` y `migrate` cargan automaticamente `backend/.env` cuando
existe. El archivo esta excluido de Git.

## Base de datos

Aplicar las migraciones en orden sobre PostgreSQL con SSL habilitado. La
migracion inicial crea nodos, mediciones, estado actual y eventos de alerta.
El payload crudo se conserva en `telemetry_measurements.raw_payload` para
auditar que la decodificacion coincida con el mensaje recibido.

## Pruebas

```text
npm.cmd test
```