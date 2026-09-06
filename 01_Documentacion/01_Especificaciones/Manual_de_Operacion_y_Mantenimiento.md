# 01 - Manual de Operación y Mantenimiento de Campo
**EMPRESA BIOSIG — DEPARTAMENTO DE INGENIERÍA Y DESARROLLO IoT**  
**Proyecto:** Sistema Sensagri (Telemetría Térmica Foliar y Alerta de Heladas)  
**Versión del Documento:** v2.0 (Actualizada para Despliegue en Campo)  

---

## 1. Herramientas e Insumos Requeridos para Instalación
Antes de acudir al predio agrícola, el técnico instalador debe contar con el siguiente equipamiento:

* **Herramientas de Montaje:** Llave fija para prensaestopas PG7/PG9, juego de destornilladores de precisión, alicates de corte diagonal, cinta métrica (mínimo 5 m).
* **Instrumentos de Medición:** Multímetro digital con puntas finas (medición de VDC y continuidad).
* **Insumos de Fijación e Intemperie:** Amarras plásticas de alta resistencia con protección UV (negras), cinta vulcanizada autosoldable, cinta aislante de vinilo, bolsitas de gel de sílice (desecante higroscópico) de 5 g a 10 g.
* **Insumos de Limpieza Técnica:** Alcohol isopropílico al 99%, paño de microfibra antiestático, hisopos de algodón de cabeza fina.

---

## 2. Protocolo de Instalación en Terreno (Paso a Paso)

### 2.1. Montaje y Hermeticidad del Gabinete Principal (IP65)
1. **Ubicación Física:** Fijar la caja estanca IP65 en la estructura firme del tronco o soporte central del frutal, a una altura mínima de **1.2 a 1.5 metros** sobre el nivel del suelo para evitar humedad por vegetación baja o salpicaduras directas de riego.
2. **Orientación de Salida de Cables:** La caja debe montarse obligatoriamente con las perforaciones de los prensaestopas **mirando hacia abajo** (dirección al suelo). Esto evita la acumulación de agua por escorrentía sobre las gomas de sellado.
3. **Ajuste de Prensacables IP68:** 
   * **PG7:** Para el cable del Panel Solar y la sonda de suelo.
   * **PG9:** Para la manguera exterior del cable Ethernet Cat6 Outdoor.
   * Aparejar la tuerca de compresión con la goma de sello aplastando firmemente la cubierta exterior del cable hasta que no exista juego mecánico.
4. **Protección Anti-Humedad Interna:** Antes de realizar el cierre definitivo del gabinete, pegar una bolsa de gel de sílice en un costado interior seco (utilizando cinta doble contacto) sin tocar los pines descubiertos de la PCB.

---

### 2.2. Orientación e Inclinación del Panel Solar (5V / 1W)
1. **Orientación Acimutal:** Orientar la cara receptora del panel estrictamente hacia el **Norte geográfico** (Hemisferio Sur) utilizando brújula o GPS.
2. **Ángulo de Inclinación:** Ajustar el soporte a la latitud del predio (aproximadamente **35° de inclinación** respecto a la horizontal para las regiones del Maule y O'Higgins). Este ángulo optimiza la captación durante los meses de invierno cuando el sol alcanza su altura angular más baja.
3. **Punto de Sombra:** Asegurar que la ubicación elegida esté libre de sombras proyectadas por ramas superiores o estructuras colindantes entre las 09:00 y las 17:00 horas.

---

### 2.3. Despliegue del Arnés de Sensores Infrarrojos Foliar (MLX90614)
1. **Zonificación en la Canopia:**
   * **Estrato Superior (Copa Alta):** Instalar el sensor N° 1 (Canal 0) en la periferia superior del árbol, apuntando a yemas expuestas.
   * **Estrato Medio (Copa Media):** Instalar el sensor N°
   * 2 (Canal 1) en la zona densa intermedia.
   * **Estrato Inferior (Copa Baja):** Instalar el sensor N° 3 (Canal 2) en la rama más baja expuesta a la corriente de aire frío del suelo (~1.0 m sobre el nivel del suelo).
2. **Ajuste de Viseras 3D (Filamento ASA):**
   * Fijar los capuchones impresos en 3D en la rama mediante abrazaderas plásticas UV.
   * Orientar la lente del sensor MLX90614 con un ángulo de **45° hacia abajo** apuntando directamente al tejido vegetal/yema a una distancia de **10 a 20 cm**.
   * **Regla Crítica:** La lente no debe quedar mirando directamente hacia arriba (para evitar acumulación de polvo/agua) ni orientada hacia el sol de frente.
3. **Canalización del Cable Cat6:** Guiar el cable Ethernet Cat6 fijándolo al tronco mediante amarras plásticas cada 30 cm, evitando tirones o curvas con radio inferior a 5 cm.

---

### 2.4. Instalación de Sensado de Suelo y Microclima Ambiental
1. **Sonda Capacitiva de Suelo (Humedad):** Insertar la placa capacitiva verticalmente en la zona activa de raíces (a 15–20 cm de profundidad) bajo la proyección de la copa. No golpear la placa directamente con martillo; abrir un perfil previo en la tierra.
2. **Sonda Térmica DS18B20 (Temperatura de Suelo):** Enterrar la sonda metálica junto a la sonda capacitiva a la misma profundidad. ⚠️ NO implementada en firmware actual (solo humedad capacitiva vía ADC).
3. **Sensor Ambiental BME280 (Humedad/Presión/Temp Base):** Montar el sensor alojado dentro de su pantalla de radiación de platos apilados (rejilla 3D) en la cara inferior exterior de la caja estanca IP65, protegido de la luz solar directa y salpicaduras.

---

### 2.5. Encendido y Verificación de Puesta en Marcha
1. **Verificación de Voltaje:** Abrir el gabinete, verificar que la batería Li-Ion 18650 registre entre **3.7V y 4.2V DC** en sus borneras.
2. **Secuencia de Encendido:** Accionar el interruptor *Rocker* de alimentación a la posición `ON`.
3. **Confirmación Visual (LEDs):**
   * El microcontrolador XIAO ESP32-S3 encenderá un destello LED breve indicando el arranque de la FSM.
   * Durante el envío LoRa, el módulo emite una ráfaga de actividad de transmisión.
4. **Verificación en Receptor:** Confirmar la recepción de la trama binaria en la pantalla/log del receptor de galpón o la plataforma de The Things Network (TTN).

---

## 3. Protocolo de Mantenimiento Preventivo (Frecuencia Semestral)

Para asegurar la continuidad operativa de la estación de telemetría, se debe ejecutar el siguiente protocolo cada 6 meses (idealmente previo al inicio del periodo de heladas en otoño e inicio de primavera):

### 3.1. Mantenimiento Mecánico y Estanqueidad
* **Revisión de Juntas Toricas y Empaquetaduras:** Abrir el gabinete IP65 e inspeccionar el estado de la goma perimetral. Si presenta grietas o deformación, aplicar una fina película de grasa de silicona o reemplazar.
* **Apriete de Prensacables:** Comprobar manualmente que las tuercas compresoras de los prensaestopas PG7/PG9 sigan firmes.
* **Reemplazo de Desecante:** Reemplazar la bolsa de gel de sílice interior por una nueva reactivada/seca.
* **Inspección de Viseras 3D:** Verificar que el filamento ASA no presente cristalización extrema ni fracturas provocadas por viento o roce de ramas.

### 3.2. Mantenimiento Óptico y Sensórico
* **Limpieza de Lentes Infrarrojas (MLX90614):** Humedecer un hisopo de algodón con alcohol isopropílico y limpiar suavemente la ventana metálica del sensor MLX90614 para retirar polvo, tela de araña o residuos orgánicos. **No utilizar solventes abrasivos.**
* **Limpieza del Panel Solar:** Limpiar la cara de vidrio del panel fotovoltaico con un paño de microfibra humedecido con agua limpia para eliminar el polvo en suspensión o guano de aves que reduzca la eficiencia de carga.
* **Limpieza de Pantalla BME280:** Retirar insectos o suciedad acumulada en las ranuras de ventilación de la rejilla inferior del gabinete.

### 3.3. Mantenimiento Eléctrico y Energético
* **Medición de Baterías:** Mapear con multímetro el voltaje de vaciado del banco Li-Ion (debe estar en el rango $3.3\,\text{V} - 4.2\,\text{V}$). Si el voltaje se encuentra bajo los $3.0\,\text{V}$, reemplazar las celdas.
* **Inspección de Conexiones I2C:** Inspeccionar que no exista sulfatación ni humedad en los bornes a tornillo de la placa perforada verde o conectores P82B715.

---

## 4. Guía de Diagnóstico de Fallas Frecuentes (Troubleshooting)

| Sintoma / Fallo | Causa Probable | Procedimiento de Corrección |
| :--- | :--- | :--- |
| **Pérdida total de datos en la nube o receptor.** | 1. Batería agotada ($< 3.0\,\text{V}$).<br>2. Interruptor en `OFF`.<br>3. Fallo de cobertura LoRa. | 1. Mide voltaje en borneras $B+/B-$.<br>2. Revisa fusible/switch.<br>3. Verifica la orientación de la antena LoRa a 915 MHz. |
| **Línea de sensor IR reporta $0.0\,\text{^\circ C}$ o $-273.15\,\text{^\circ C}$.** | Error en el bus I2C / Canal del multiplexor no responde. | 1. Revisa continuidad en cables $Lx/Ly$ del extensor P82B715.<br>2. Comprueba que el extensor P82B715 reciba sus $5\,\text{V}$ de alimentación.<br>3. Limpia la dirección I2C `0x70` del PCA9548A. |
| **La batería no carga durante el día.** | 1. Diodo Schottky invertido.<br>2. Suciedad sobre el panel.<br>3. Desconexión en pad $IN+ / IN-$. | 1. Revisa que la franja plateada del 1N5817 apunte hacia el pad $IN+$ del TP4056.<br>2. Limpia la superficie solar.<br>3. Mide voltaje a la salida del panel ($> 4.8\,\text{V}$ a pleno sol). |
| **Lecturas de temperatura foliar inusualmente altas ($> 40\,\text{^\circ C}$).** | Sol directo golpeando la lente del sensor MLX90614. | Reorienta la visera 3D a $45^\circ$ hacia abajo asegurando que la copa del árbol bloquee la radiación solar directa. |
| **Lectura estancada en humedad de suelo.** | Corrosión o cable cortado en pin analógico $D6$ (ADC_SOIL_MOISTURE_PIN). | Verifica la integridad del conector de 3 pines y prueba la lectura en vaso de agua usando el sketch de prueba `Test_ADC_Suelo_Bateria.ino`. |
