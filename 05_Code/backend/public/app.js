const state = { nodes: [], selectedNodeId: '', hours: 24 };
const $ = (id) => document.getElementById(id);

async function getJson(path) {
  const response = await fetch(path);
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

function formatDate(value) {
  return value ? new Date(value).toLocaleString('es-CL', { dateStyle: 'short', timeStyle: 'short' }) : '--';
}

function formatTime(value) {
  return value ? new Date(value).toLocaleTimeString('es-CL', { hour: '2-digit', minute: '2-digit' }) : '--';
}

function renderNodeTabs() {
  const container = $('nodeTabsContainer');
  if (!container) return;

  if (!state.nodes.length) {
    container.innerHTML = '<p class="empty">Sin nodos registrados en la red.</p>';
    return;
  }

  const tabsHtml = state.nodes.map((node) => {
    const nodeId = node.nodeId ?? node.node_id;
    const displayName = node.displayName ?? node.display_name;
    const lastReceived = node.lastReceivedAt ?? node.last_received_at;
    const connState = node.connectionState ?? node.connection_state;
    
    const isSelected = String(nodeId) === String(state.selectedNodeId);
    const isOnline = connState === 'online';
    
    return `
      <div class="node-tab ${isSelected ? 'active' : ''}" data-node-id="${nodeId}">
        <div class="tab-header">
          <span class="tab-title">${displayName}</span>
          <span class="tab-badge">#${nodeId}</span>
        </div>
        <div class="tab-body">
          <span class="status-indicator ${isOnline ? 'online' : 'offline'}">
            <i></i> ${isOnline ? 'En línea' : 'Sin señal'}
          </span>
          <span class="tab-time">${formatTime(lastReceived)}</span>
        </div>
      </div>
    `;
  }).join('');

  container.innerHTML = tabsHtml;

  document.querySelectorAll('.node-tab').forEach((tab) => {
    tab.addEventListener('click', () => {
      state.selectedNodeId = tab.dataset.nodeId;
      renderNodeTabs();
      refresh();
    });
  });
}

function renderNodesList() {
  const list = $('nodesList');
  if (!list) return;

  list.innerHTML = state.nodes.length ? state.nodes.map((node) => {
    const nodeId = node.nodeId ?? node.node_id;
    const displayName = node.displayName ?? node.display_name;
    const seq = node.lastSequenceNumber ?? node.last_sequence_number ?? '--';
    const lastReceived = node.lastReceivedAt ?? node.last_received_at;
    const connState = node.connectionState ?? node.connection_state ?? 'unknown';

    return `
      <button class="node-row node-button" data-node-id="${nodeId}">
        <div>
          <div class="node-name">${displayName} <span class="node-id">#${nodeId}</span></div>
          <div class="node-meta">Secuencia ${seq} · ${formatDate(lastReceived)}</div>
        </div>
        <span class="pill ${connState === 'stale' ? 'stale' : ''}">${connState}</span>
      </button>
    `;
  }).join('') : '<p class="empty">Sin nodos registrados.</p>';
    
  document.querySelectorAll('.node-button').forEach((button) => 
    button.addEventListener('click', () => { 
      state.selectedNodeId = button.dataset.nodeId; 
      renderNodeTabs();
      refresh(); 
    })
  );
}

function selectedNode() { 
  return state.nodes.find((node) => String(node.nodeId ?? node.node_id) === String(state.selectedNodeId)) ?? state.nodes[0]; 
}

function setFormValue(id, value) { 
  const el = $(id);
  if (el) el.checked = Boolean(value); 
}

function renderAlertSettings(settings) {
  if (!$('alertSettingsForm')) return;

  setFormValue('frostEnabled', settings.frostEnabled ?? settings.frost_enabled); 
  if ($('frostThresholdC')) $('frostThresholdC').value = settings.frostThresholdC ?? settings.frost_threshold_c ?? 2.0;
  
  setFormValue('heatEnabled', settings.heatEnabled ?? settings.heat_enabled); 
  if ($('heatThresholdC')) $('heatThresholdC').value = settings.heatThresholdC ?? settings.heat_threshold_c ?? 25.0;
  
  setFormValue('sensorEnabled', settings.sensorEnabled ?? settings.sensor_enabled); 
  setFormValue('connectivityEnabled', settings.connectivityEnabled ?? settings.connectivity_enabled); 
  setFormValue('telegramEnabled', settings.telegramEnabled ?? settings.telegram_enabled);
  
  if ($('saveSettingsButton')) {
    $('saveSettingsButton').disabled = !(settings.nodeId ?? settings.node_id);
  }
}

async function loadAlertSettings(node) {
  if (!node) { renderAlertSettings({}); return; }
  const nodeId = node.nodeId ?? node.node_id;
  try { 
    renderAlertSettings(await getJson(`/api/nodes/${nodeId}/alert-config`)); 
    if ($('settingsStatus')) {
      $('settingsStatus').textContent = `Nodo #${nodeId}`; 
      $('settingsStatus').style.color = '';
    }
  } catch (error) { 
    if ($('settingsStatus')) $('settingsStatus').textContent = 'No disponible'; 
  }
}

function renderMetrics(node, history) {
  const validTemps = history.flatMap((item) => [
    item.tempCanopyTopC ?? item.temp_canopy_top_c, 
    item.tempCanopyMidC ?? item.temp_canopy_mid_c, 
    item.tempCanopyLowC ?? item.temp_canopy_low_c, 
    item.tempBaseC ?? item.temp_base_c
  ]).filter((val) => val !== null && val !== undefined && Number.isFinite(Number(val))).map(Number);

  const quality = node?.qualityMetadata ?? node?.quality_metadata ?? {};
  const online = state.nodes.filter((item) => (item.connectionState ?? item.connection_state) === 'online').length;
  const stale = state.nodes.filter((item) => (item.connectionState ?? item.connection_state) === 'stale').length;
  
  if ($('fieldState')) $('fieldState').textContent = state.nodes.length === 0 ? 'Sin datos' : stale > 0 ? 'Atención' : online === state.nodes.length ? 'Todos operativos' : 'Esperando datos';
  if ($('fieldDetail')) $('fieldDetail').textContent = state.nodes.length ? `${online} operativos · ${stale} sin comunicación · ${state.nodes.length} nodos registrados` : 'Esperando nodos conectados';
  if ($('minTemp')) $('minTemp').textContent = validTemps.length ? `${Math.min(...validTemps).toFixed(2)} °C` : '-- °C';
  if ($('lastSeen')) $('lastSeen').textContent = node ? formatDate(node.lastReceivedAt ?? node.last_received_at) : '--';
  if ($('lastSeenDetail')) $('lastSeenDetail').textContent = node ? `Secuencia ${node.lastSequenceNumber ?? node.last_sequence_number ?? '--'}` : '--';
  if ($('missedCount')) $('missedCount').textContent = quality.missedCount ?? quality.missed_count ?? '--';
}

function renderSensors(history) {
  const current = history.at(-1);
  const getVal = (keyCamel, keySnake) => current ? (current[keyCamel] ?? current[keySnake]) : null;

  const fields = [
    ['Copa superior', getVal('tempCanopyTopC', 'temp_canopy_top_c'), '°C'],
    ['Copa media', getVal('tempCanopyMidC', 'temp_canopy_mid_c'), '°C'],
    ['Copa baja', getVal('tempCanopyLowC', 'temp_canopy_low_c'), '°C'],
    ['Base BME280 · temperatura', getVal('tempBaseC', 'temp_base_c'), '°C'],
    ['Base BME280 · humedad', getVal('humBasePct', 'hum_base_pct'), '%'],
    ['Base BME280 · presión', getVal('pressureHpa', 'pressure_hpa'), 'hPa'],
    ['Humedad de suelo', getVal('soilMoisturePct', 'soil_moisture_pct'), '%'],
    ['Temperatura de suelo', getVal('tempSoilC', 'temp_soil_c'), '°C']
  ];

  if ($('selectedNodeLabel')) {
    const activeNode = selectedNode();
    const nodeId = activeNode ? (activeNode.nodeId ?? activeNode.node_id) : '--';
    $('selectedNodeLabel').textContent = `Nodo #${nodeId}`;
  }
  
  if ($('sensorList')) {
    $('sensorList').innerHTML = current ? `${fields.map(([label, val, unit]) => { 
      const available = val !== null && val !== undefined && Number.isFinite(Number(val)); 
      return `<div class="sensor-row"><div><div class="sensor-name">${label}</div><div class="sensor-health ${available ? 'healthy' : 'unavailable'}"><i></i>${available ? 'Sensor correcto' : 'Sin lectura válida'}</div></div><span class="sensor-value ${available ? '' : 'sensor-invalid'}">${available ? `${Number(val).toFixed(2)} ${unit}` : 'No disponible'}</span></div>`; 
    }).join('')}<div class="sensor-footnote">Lectura ${formatDate(current.receivedAt ?? current.received_at)} · Secuencia #${current.sequenceNumber ?? current.sequence_number ?? '--'} · Flags 0x${Number(current.statusFlags ?? current.status_flags ?? 0).toString(16).padStart(4, '0').toUpperCase()}</div>` : '<p class="empty">No hay registros de telemetría disponibles para este rango.</p>';
  }
}

function renderChart(history) {
  const svg = $('temperatureChart');
  if (!svg) return;  
  if ($('chartEmpty')) $('chartEmpty').hidden = history.length > 0;
  if (!history.length) { svg.innerHTML = ''; return; }

  const values = history
    .map((item) => Number(item.tempBaseC ?? item.temp_base_c))
    .filter(Number.isFinite);

  if (values.length === 0) {
    svg.innerHTML = '';
    if ($('chartEmpty')) $('chartEmpty').hidden = false;
    return;
  }

  const rawMin = Math.min(...values);
  const rawMax = Math.max(...values);
  
  const min = Math.min(-1.0, Math.floor(rawMin - 1.0));
  const max = Math.max(3.0, Math.ceil(rawMax + 1.0));
  
  const width = 800; 
  const height = 240;
  const paddingBottom = 30;
  const chartHeight = height - paddingBottom;

  const getY = (val) => chartHeight - ((val - min) / (max - min)) * chartHeight;
  const yZero = getY(0);

  const pointsData = history.map((item, index) => {
    const val = Number(item.tempBaseC ?? item.temp_base_c);
    const yVal = Number.isFinite(val) ? val : 0;
    const x = (index / Math.max(1, history.length - 1)) * (width - 20) + 10;
    const y = getY(yVal);
    const time = formatTime(item.receivedAt ?? item.received_at);
    return { x, y, val: yVal, time };
  });

  const polylinePoints = pointsData.map(p => `${p.x},${p.y}`).join(' ');

  const stepX = Math.max(1, Math.floor(history.length / 6));
  const timeLabels = pointsData.filter((_, idx) => idx % stepX === 0 || idx === pointsData.length - 1)
    .map(p => `<text x="${p.x}" y="${height - 5}" text-anchor="middle" fill="#64748b" font-size="11" font-weight="500">${p.time}</text>`)
    .join('');

  const dotsHtml = pointsData.map(p => `
    <circle cx="${p.x}" cy="${p.y}" r="4" fill="${p.val <= 0 ? '#ef4444' : '#0284c7'}" stroke="#ffffff" stroke-width="2">
      <title>${p.time} | ${p.val.toFixed(2)} °C</title>
    </circle>
  `).join('');

  svg.innerHTML = `
    <rect x="0" y="${yZero}" width="${width}" height="${chartHeight - yZero}" fill="rgba(239, 68, 68, 0.08)" />
    <line class="chart-grid" x1="0" y1="${getY(max)}" x2="${width}" y2="${getY(max)}" stroke="#e2e8f0" stroke-dasharray="2 2" />
    <line class="chart-grid" x1="0" y1="${getY((max + min) / 2)}" x2="${width}" y2="${getY((max + min) / 2)}" stroke="#e2e8f0" stroke-dasharray="2 2" />
    <line class="chart-grid" x1="0" y1="${chartHeight}" x2="${width}" y2="${chartHeight}" stroke="#cbd5e1" />

    <line x1="0" y1="${yZero}" x2="${width}" y2="${yZero}" stroke="#ef4444" stroke-width="1.8" stroke-dasharray="5 4" />
    <text x="${width - 10}" y="${yZero - 6}" text-anchor="end" fill="#ef4444" font-size="11" font-weight="bold">0.0 °C (Umbral Helada)</text>

    <polyline fill="none" stroke="#0284c7" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" points="${polylinePoints}" />
    ${dotsHtml}

    <text class="chart-label" x="8" y="${getY(max) + 12}" fill="#475569" font-size="11" font-weight="600">${max.toFixed(1)} °C</text>
    <text class="chart-label" x="8" y="${getY(min) - 6}" fill="#475569" font-size="11" font-weight="600">${min.toFixed(1)} °C</text>
    ${timeLabels}
  `;
}

function renderTelemetryTable(history) {
  const tbody = $('telemetryTableBody');
  if (!tbody) return;

  if (!history || history.length === 0) {
    tbody.innerHTML = '<tr><td colspan="9" class="table-empty">No hay registros de telemetría disponibles para este rango.</td></tr>';
    return;
  }

  const rows = [...history].reverse().map((m) => {
    const nodeId = m.nodeId ?? m.node_id ?? '--';
    const top = m.tempCanopyTopC ?? m.temp_canopy_top_c;
    const mid = m.tempCanopyMidC ?? m.temp_canopy_mid_c;
    const low = m.tempCanopyLowC ?? m.temp_canopy_low_c;
    const base = m.tempBaseC ?? m.temp_base_c;
    const hum = m.humBasePct ?? m.hum_base_pct;
    const soil = m.tempSoilC ?? m.temp_soil_c;
    const seq = m.sequenceNumber ?? m.sequence_number ?? '--';
    const dateStr = formatDate(m.receivedAt ?? m.received_at);

    const isFrost = [top, mid, low, base].some(val => val !== null && val !== undefined && Number(val) <= 0);

    return `
      <tr class="${isFrost ? 'row-frost' : ''}">
        <td class="cell-seq">Nodo #${nodeId}</td>
        <td class="cell-time">${dateStr}</td>
        <td class="cell-seq">#${seq}</td>
        <td>${top !== null && top !== undefined ? Number(top).toFixed(2) + ' °C' : '--'}</td>
        <td>${mid !== null && mid !== undefined ? Number(mid).toFixed(2) + ' °C' : '--'}</td>
        <td>${low !== null && low !== undefined ? Number(low).toFixed(2) + ' °C' : '--'}</td>
        <td class="cell-base ${isFrost ? 'temp-alert' : ''}">${base !== null && base !== undefined ? Number(base).toFixed(2) + ' °C' : '--'}</td>
        <td>${hum !== null && hum !== undefined ? Number(hum).toFixed(2) + ' %' : '--'}</td>
        <td>${soil !== null && soil !== undefined ? Number(soil).toFixed(2) + ' °C' : '--'}</td>
      </tr>
    `;
  }).join('');

  tbody.innerHTML = rows;
}

function renderAlerts(alerts) {
  const container = $('alertsList');
  if (!container) return;

  container.innerHTML = alerts && alerts.length ? alerts.slice(0, 8).map((alert) => `
    <div class="alert-row">
      <div>
        <div class="node-name">Nodo #${alert.nodeId ?? alert.node_id} · ${alert.alertType ?? alert.alert_type}</div>
        <div class="alert-meta">${alert.message} · ${formatDate(alert.startedAt ?? alert.started_at)}</div>
      </div>
      <span class="pill ${alert.state === 'active' ? 'alert' : ''}">${alert.state}</span>
    </div>`).join('') : '<p class="empty">No hay alertas registradas.</p>';
}

async function refresh() {
  try {
    const [health, nodes, alerts] = await Promise.all([
      getJson('/api/health').catch(() => ({ database: 'error' })), 
      getJson('/api/nodes').catch(() => []), 
      getJson('/api/alerts').catch(() => [])
    ]);

    state.nodes = nodes; 
    
    if (!state.selectedNodeId && state.nodes.length > 0) {
      state.selectedNodeId = String(state.nodes[0].nodeId ?? state.nodes[0].node_id);
    }

    renderNodeTabs();
    renderNodesList();
    renderAlerts(alerts);

    const node = selectedNode(); 
    await loadAlertSettings(node);

    const nodeId = node ? (node.nodeId ?? node.node_id) : null;
    const history = nodeId ? await getJson(`/api/nodes/${nodeId}/history?hours=${state.hours}`).catch(() => []) : [];
    
    renderMetrics(node, history); 
    renderSensors(history); 
    renderChart(history); 
    renderTelemetryTable(history);

    if ($('healthDot')) $('healthDot').className = 'status-dot good'; 
    if ($('healthText')) $('healthText').textContent = health.database === 'ok' ? 'Base conectada' : 'Sin base de datos'; 
    if ($('updatedText')) $('updatedText').textContent = `Actualizado ${new Date().toLocaleTimeString('es-CL')}`;
  } catch (error) {
    if ($('healthDot')) $('healthDot').className = 'status-dot bad'; 
    if ($('healthText')) $('healthText').textContent = 'Backend no disponible'; 
    if ($('updatedText')) $('updatedText').textContent = error.message;
  }
}

document.querySelectorAll('#financeTimeControls .time-btn').forEach((btn) => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('#financeTimeControls .time-btn').forEach((b) => b.classList.remove('active'));
    btn.classList.add('active');
    state.hours = Number(btn.dataset.hours);
    refresh();
  });
});

if ($('refreshButton')) {
  $('refreshButton').addEventListener('click', refresh);
}

if ($('alertSettingsForm')) {
  $('alertSettingsForm').addEventListener('submit', async (event) => {
    event.preventDefault(); 
    const node = selectedNode(); 
    if (!node) return;

    const nodeId = node.nodeId ?? node.node_id;
    const button = $('saveSettingsButton'); 
    const statusLabel = $('settingsStatus');

    if (button) button.disabled = true; 
    if (statusLabel) {
      statusLabel.textContent = 'Guardando en BD...';
      statusLabel.style.color = '#087f73';
    }

    try {
      const updated = await fetch(`/api/nodes/${nodeId}/alert-config`, { 
        method: 'PUT', 
        headers: { 'Content-Type': 'application/json' }, 
        body: JSON.stringify({
          frostEnabled: $('frostEnabled')?.checked ?? false, 
          frostThresholdC: Number($('frostThresholdC')?.value ?? 0), 
          heatEnabled: $('heatEnabled')?.checked ?? false, 
          heatThresholdC: Number($('heatThresholdC')?.value ?? 35), 
          sensorEnabled: $('sensorEnabled')?.checked ?? false, 
          connectivityEnabled: $('connectivityEnabled')?.checked ?? false, 
          telegramEnabled: $('telegramEnabled')?.checked ?? false,
        }) 
      }).then((response) => { 
        if (!response.ok) throw new Error('Error guardando en BD'); 
        return response.json(); 
      });

      if (statusLabel) {
        statusLabel.textContent = '✓ Configuración Guardada';
        statusLabel.style.color = '#16a34a';
      }

      renderAlertSettings(updated);

      setTimeout(() => {
        if (statusLabel) {
          statusLabel.textContent = `Nodo #${nodeId}`;
          statusLabel.style.color = '';
        }
      }, 3000);

    } catch (error) { 
      if (statusLabel) {
        statusLabel.textContent = '❌ Error al guardar';
        statusLabel.style.color = '#dc2626';
      }
    } finally {
      if (button) button.disabled = false;
    }
  });
}

// Carga inicial y refresco automático cada 60 segundos
refresh();
setInterval(refresh, 60000);