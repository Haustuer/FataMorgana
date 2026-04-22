#pragma once

#include <pgmspace.h>

static const char kStatusPageHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FataMorgana ESP Config</title>
  <style>
    :root {
      color-scheme: dark;
      font-family: Arial, sans-serif;
    }

    body {
      margin: 0;
      padding: 20px;
      background: #111827;
      color: #f3f4f6;
    }

    h1, h2 {
      margin-top: 0;
    }

    p {
      color: #cbd5e1;
    }

    .layout {
      display: grid;
      gap: 16px;
    }

    .grid {
      display: grid;
      gap: 12px;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    }

    .card {
      background: #1f2937;
      border: 1px solid #374151;
      border-radius: 12px;
      padding: 16px;
    }

    .label {
      font-size: 0.8rem;
      color: #93c5fd;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .value {
      margin-top: 6px;
      font-size: 1.05rem;
      font-weight: 600;
      word-break: break-word;
    }

    .form-grid {
      display: grid;
      gap: 12px;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    }

    label {
      display: grid;
      gap: 6px;
      color: #cbd5e1;
    }

    input,
    select,
    button {
      font: inherit;
      padding: 10px 12px;
      border-radius: 8px;
      border: 1px solid #475569;
      background: #020617;
      color: #f3f4f6;
    }

    .checkbox {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-top: 12px;
    }

    .checkbox input {
      width: auto;
      margin: 0;
    }

    button {
      background: linear-gradient(135deg, #2563eb, #7c3aed);
      border: 0;
      cursor: pointer;
    }

    button:hover {
      opacity: 0.9;
    }

    .hidden {
      display: none;
    }

    h3 {
      margin: 16px 0 8px 0;
      color: #93c5fd;
      font-size: 0.9rem;
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }

    #saveMessage {
      margin-top: 12px;
      padding: 12px;
      border-radius: 8px;
      background: #020617;
      white-space: pre-wrap;
      color: #cbd5e1;
    }
  </style>
</head>
<body>
  <h1>FataMorgana ESP Config</h1>
  <p>Choose how the incoming image should be mapped onto the LED strip.</p>

  <div class="layout">
    <section class="card">
      <h2>Mapping Configuration</h2>
      <form id="configForm">
        <div class="form-grid">
          <label>
            Mode
            <select id="mode">
              <option value="0">Row</option>
              <option value="1">Column</option>
              <option value="2">Rectangle</option>
            </select>
          </label>

          <label>
            Sample Mode
            <select id="sampleMode">
              <option value="0">Pixel</option>
              <option value="1">Interpolated</option>
            </select>
          </label>

          <label id="rowField">
            Row Index
            <input id="rowIndex" type="number" min="0" step="1">
          </label>

          <label id="columnField">
            Column Index
            <input id="columnIndex" type="number" min="0" step="1">
          </label>

          <label id="linePixelsField">
            Output Pixels
            <input id="linePixels" type="number" min="1" step="1">
          </label>

          <label id="rectXField">
            Rectangle X
            <input id="rectX" type="number" min="0" step="1">
          </label>

          <label id="rectYField">
            Rectangle Y
            <input id="rectY" type="number" min="0" step="1">
          </label>

          <label id="rectWidthField">
            Rectangle Width
            <input id="rectWidth" type="number" min="1" step="1">
          </label>

          <label id="rectHeightField">
            Rectangle Height
            <input id="rectHeight" type="number" min="1" step="1">
          </label>

          <label id="serpentineField">
            Serpentine Mode
            <select id="serpentine">
              <option value="0">None</option>
              <option value="1">Horizontal (zigzag L-R)</option>
              <option value="2">Vertical (zigzag U-D)</option>
            </select>
          </label>

          <label id="rotationField">
            Rotation
            <select id="rotation">
              <option value="0">0° (No rotation)</option>
              <option value="1">90° (Clockwise)</option>
              <option value="2">180°</option>
              <option value="3">270° (Counter-CW)</option>
            </select>
          </label>
        </div>

        <h3 id="transformHeader" class="hidden">Transform Options</h3>
        <div class="form-grid" id="transformFields" class="hidden">
          <label class="checkbox">
            <input id="flipX" type="checkbox">
            <span>Flip X (Horizontal mirror)</span>
          </label>

          <label class="checkbox">
            <input id="flipY" type="checkbox">
            <span>Flip Y (Vertical mirror)</span>
          </label>

          <label class="checkbox">
            <input id="flipZ" type="checkbox">
            <span>Flip Z (Transpose/Diagonal)</span>
          </label>
        </div>

        <button type="submit">Apply Now</button>
      </form>

      <div id="saveMessage">Loading current config...</div>
      <p style="color: #93c5fd; font-size: 0.85rem; margin-top: 8px;">💡 Changes apply automatically as you adjust settings</p>
    </section>

    <section class="card">
      <h2>Status</h2>
      <div class="grid">
        <div><div class="label">IP Address</div><div class="value" id="ip">-</div></div>
        <div><div class="label">WiFi Status</div><div class="value" id="wifiStatus">-</div></div>
        <div><div class="label">WebSocket</div><div class="value" id="wsStatus">Connecting...</div></div>
        <div><div class="label">UDP Port</div><div class="value" id="udpPort">-</div></div>
        <div><div class="label">LED Count</div><div class="value" id="ledCount">-</div></div>
        <div><div class="label">Mapping</div><div class="value" id="mapping">-</div></div>
        <div><div class="label">Mode</div><div class="value" id="mappingMode">-</div></div>
        <div><div class="label">Sample Mode</div><div class="value" id="sampleModeStatus">-</div></div>
        <div><div class="label">Last Frame</div><div class="value" id="lastFrame">-</div></div>
        <div><div class="label">Image Size</div><div class="value" id="imageSize">-</div></div>
        <div><div class="label">RGB Type</div><div class="value" id="rgbType">-</div></div>
        <div><div class="label">Chunks</div><div class="value" id="chunks">-</div></div>
        <div><div class="label">Packets</div><div class="value" id="packets">-</div></div>
        <div><div class="label">Rendered Frames</div><div class="value" id="renderedFrames">-</div></div>
        <div><div class="label">Last Render</div><div class="value" id="lastRender">-</div></div>
      </div>
    </section>
  </div>

  <script>
    const form = document.getElementById('configForm');
    const modeField = document.getElementById('mode');
    const sampleModeField = document.getElementById('sampleMode');
    const rowIndexField = document.getElementById('rowIndex');
    const columnIndexField = document.getElementById('columnIndex');
    const linePixelsField = document.getElementById('linePixels');
    const rectXField = document.getElementById('rectX');
    const rectYField = document.getElementById('rectY');
    const rectWidthField = document.getElementById('rectWidth');
    const rectHeightField = document.getElementById('rectHeight');
    const serpentineField = document.getElementById('serpentine');
    const rotationField = document.getElementById('rotation');
    const flipXField = document.getElementById('flipX');
    const flipYField = document.getElementById('flipY');
    const flipZField = document.getElementById('flipZ');
    const saveMessage = document.getElementById('saveMessage');

    let ws = null;
    let reconnectTimer = null;

    function setText(id, value) {
      document.getElementById(id).textContent = value;
    }

    function updateModeVisibility() {
      const mode = Number(modeField.value);
      const lineMode = mode === 0 || mode === 1;
      const rowMode = mode === 0;
      const columnMode = mode === 1;
      const rectangleMode = mode === 2;

      document.getElementById('rowField').classList.toggle('hidden', !rowMode);
      document.getElementById('columnField').classList.toggle('hidden', !columnMode);
      document.getElementById('linePixelsField').classList.toggle('hidden', !lineMode);
      document.getElementById('rectXField').classList.toggle('hidden', !rectangleMode);
      document.getElementById('rectYField').classList.toggle('hidden', !rectangleMode);
      document.getElementById('rectWidthField').classList.toggle('hidden', !rectangleMode);
      document.getElementById('rectHeightField').classList.toggle('hidden', !rectangleMode);
      document.getElementById('serpentineField').classList.toggle('hidden', !rectangleMode);
      document.getElementById('rotationField').classList.toggle('hidden', !rectangleMode);
      document.getElementById('transformHeader').classList.toggle('hidden', !rectangleMode);
      document.getElementById('transformFields').classList.toggle('hidden', !rectangleMode);
      sampleModeField.disabled = !lineMode;
    }

    function applyConfig(config) {
      modeField.value = config.mode;
      sampleModeField.value = config.sampleMode;
      rowIndexField.value = config.rowIndex;
      columnIndexField.value = config.columnIndex;
      linePixelsField.value = config.linePixels;
      linePixelsField.max = config.ledCount;
      rectXField.value = config.rectX;
      rectYField.value = config.rectY;
      rectWidthField.value = config.rectWidth;
      rectHeightField.value = config.rectHeight;
      serpentineField.value = config.serpentine || 0;
      rotationField.value = config.rotation || 0;
      flipXField.checked = Boolean(config.flipX);
      flipYField.checked = Boolean(config.flipY);
      flipZField.checked = Boolean(config.flipZ);
      updateModeVisibility();
    }

    async function loadConfig() {
      const response = await fetch('/config.json', { cache: 'no-store' });
      const config = await response.json();
      applyConfig(config);
      saveMessage.textContent = 'Config loaded.';
    }

    async function refreshStatus() {
      try {
        const response = await fetch('/status.json', { cache: 'no-store' });
        const status = await response.json();

        setText('ip', status.ip);
        setText('wifiStatus', status.wifiStatus);
        setText('udpPort', status.udpPort);
        setText('ledCount', status.ledCount);
        setText('mapping', status.mapping);
        setText('mappingMode', status.mappingMode);
        setText('sampleModeStatus', status.sampleMode);
        setText('lastFrame', status.lastFrame);
        setText('imageSize', status.imageSize);
        setText('rgbType', status.rgbType);
        setText('chunks', status.chunks);
        setText('packets', status.packets);
        setText('renderedFrames', status.renderedFrames);
        setText('lastRender', status.lastRender);
      } catch (error) {
        setText('wifiStatus', 'status fetch failed');
      }
    }

    async function saveConfig(showMessage = true) {
      if (showMessage) {
        saveMessage.textContent = 'Saving config...';
      }

      const payload = {
        mode: Number(modeField.value),
        sampleMode: Number(sampleModeField.value),
        rowIndex: Number(rowIndexField.value || 0),
        columnIndex: Number(columnIndexField.value || 0),
        linePixels: Number(linePixelsField.value || 1),
        rectX: Number(rectXField.value || 0),
        rectY: Number(rectYField.value || 0),
        rectWidth: Number(rectWidthField.value || 1),
        rectHeight: Number(rectHeightField.value || 1),
        serpentine: Number(serpentineField.value || 0),
        rotation: Number(rotationField.value || 0),
        flipX: flipXField.checked,
        flipY: flipYField.checked,
        flipZ: flipZField.checked
      };

      try {
        const response = await fetch('/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });

        if (!response.ok) {
          throw new Error('Save failed');
        }

        const config = await response.json();
        applyConfig(config);
        await refreshStatus();
        if (showMessage) {
          saveMessage.textContent = 'Config saved.';
        }
      } catch (error) {
        if (showMessage) {
          saveMessage.textContent = error.message;
        }
      }
    }

    let autoSaveTimer = null;
    function autoSave() {
      // Show "updating..." message
      saveMessage.textContent = 'Auto-updating...';

      // Debounce: wait 300ms after last change before saving
      if (autoSaveTimer) {
        clearTimeout(autoSaveTimer);
      }
      autoSaveTimer = setTimeout(async () => {
        await saveConfig(false);
        saveMessage.textContent = 'Updated.';
        // Clear message after 2 seconds
        setTimeout(() => {
          if (saveMessage.textContent === 'Updated.') {
            saveMessage.textContent = '';
          }
        }, 2000);
      }, 300);
    }

    modeField.addEventListener('change', updateModeVisibility);

    // Auto-save on any config change
    modeField.addEventListener('change', autoSave);
    sampleModeField.addEventListener('change', autoSave);
    rowIndexField.addEventListener('input', autoSave);
    columnIndexField.addEventListener('input', autoSave);
    linePixelsField.addEventListener('input', autoSave);
    rectXField.addEventListener('input', autoSave);
    rectYField.addEventListener('input', autoSave);
    rectWidthField.addEventListener('input', autoSave);
    rectHeightField.addEventListener('input', autoSave);
    serpentineField.addEventListener('change', autoSave);
    rotationField.addEventListener('change', autoSave);
    flipXField.addEventListener('change', autoSave);
    flipYField.addEventListener('change', autoSave);
    flipZField.addEventListener('change', autoSave);

    form.addEventListener('submit', async (event) => {
      event.preventDefault();
      await saveConfig(true);
    });

    function connectWebSocket() {
      if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) {
        return;
      }

      const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
      const wsUrl = `${protocol}//${location.host}/ws`;

      try {
        ws = new WebSocket(wsUrl);

        ws.onopen = () => {
          console.log('WebSocket connected');
          setText('wsStatus', '🟢 Connected');
          saveMessage.textContent = 'Live updates enabled';
          if (reconnectTimer) {
            clearTimeout(reconnectTimer);
            reconnectTimer = null;
          }
        };

        ws.onmessage = (event) => {
          try {
            const status = JSON.parse(event.data);

            setText('ip', status.ip);
            setText('wifiStatus', status.wifiStatus);
            setText('udpPort', status.multicastPort);
            setText('ledCount', status.ledCount);
            setText('mapping', status.mapping);
            setText('mappingMode', status.mappingMode);
            setText('sampleModeStatus', status.sampleMode);
            setText('lastFrame', status.lastFrame);
            setText('imageSize', status.imageSize);
            setText('rgbType', status.rgbType);
            setText('chunks', status.chunks);
            setText('packets', status.packets);
            setText('renderedFrames', status.renderedFrames);
            setText('lastRender', status.lastRender);
          } catch (error) {
            console.error('WebSocket message error:', error);
          }
        };

        ws.onerror = (error) => {
          console.error('WebSocket error:', error);
          setText('wsStatus', '🔴 Error');
        };

        ws.onclose = () => {
          console.log('WebSocket disconnected');
          setText('wsStatus', '🟡 Reconnecting...');
          ws = null;
          // Attempt to reconnect after 2 seconds
          if (!reconnectTimer) {
            reconnectTimer = setTimeout(connectWebSocket, 2000);
          }
        };
      } catch (error) {
        console.error('Failed to create WebSocket:', error);
        setText('wsStatus', '🔴 Failed');
        if (!reconnectTimer) {
          reconnectTimer = setTimeout(connectWebSocket, 2000);
        }
      }
    }

    loadConfig().then(refreshStatus);
    connectWebSocket();

    // Fallback polling in case WebSocket is not available
    setInterval(() => {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        refreshStatus();
      }
    }, 2000);
  </script>
</body>
</html>
)HTML";
