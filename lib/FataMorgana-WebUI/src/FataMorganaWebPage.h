/**
 * @file FataMorganaWebPage.h
 * @brief HTML Configuration Page for FataMorgana Web Interface
 */

#pragma once

#include <pgmspace.h>

static const char FATAMORGANA_WEB_PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FataMorgana ESP Config</title>
  <style>
    :root {color-scheme:dark;font-family:Arial,sans-serif}
    body{margin:0;padding:20px;background:#111827;color:#f3f4f6}
    h1,h2{margin-top:0}
    p{color:#cbd5e1}
    .layout{display:grid;gap:16px}
    .grid{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(220px,1fr))}
    .card{background:#1f2937;border:1px solid #374151;border-radius:12px;padding:16px}
    .label{font-size:.8rem;color:#93c5fd;text-transform:uppercase;letter-spacing:.08em}
    .value{margin-top:6px;font-size:1.05rem;font-weight:600;word-break:break-word}
    .form-grid{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(180px,1fr))}
    label{display:grid;gap:6px;color:#cbd5e1}
    input,select,button{font:inherit;padding:10px 12px;border-radius:8px;border:1px solid #475569;background:#020617;color:#f3f4f6}
    .checkbox{display:flex;align-items:center;gap:10px;margin-top:12px}
    .checkbox input{width:auto;margin:0}
    button{background:linear-gradient(135deg,#2563eb,#7c3aed);border:0;cursor:pointer}
    button:hover{opacity:.9}
    .hidden{display:none}
    h3{margin:16px 0 8px 0;color:#93c5fd;font-size:.9rem;text-transform:uppercase;letter-spacing:.05em}
    #saveMessage{margin-top:12px;padding:12px;border-radius:8px;background:#020617;white-space:pre-wrap;color:#cbd5e1}
  </style>
</head>
<body>
  <h1>FataMorgana Config</h1>
  <p>Configure LED mapping and transforms</p>
  <div class="layout">
    <section class="card">
      <h2>Mapping Configuration</h2>
      <form id="configForm">
        <div class="form-grid">
          <label>Mode<select id="mode"><option value="0">Row</option><option value="1">Column</option><option value="2">Rectangle</option></select></label>
          <label>Sample Mode<select id="sampleMode"><option value="0">Pixel</option><option value="1">Interpolated</option></select></label>
          <label id="rowField">Row Index<input id="rowIndex" type="number" min="0" step="1"></label>
          <label id="columnField">Column Index<input id="columnIndex" type="number" min="0" step="1"></label>
          <label id="linePixelsField">Output Pixels<input id="linePixels" type="number" min="1" step="1"></label>
          <label id="rectXField">Rectangle X<input id="rectX" type="number" min="0" step="1"></label>
          <label id="rectYField">Rectangle Y<input id="rectY" type="number" min="0" step="1"></label>
          <label id="rectWidthField">Rectangle Width<input id="rectWidth" type="number" min="1" step="1"></label>
          <label id="rectHeightField">Rectangle Height<input id="rectHeight" type="number" min="1" step="1"></label>
          <label id="serpentineField">Serpentine Mode<select id="serpentine"><option value="0">None</option><option value="1">Horizontal (zigzag L-R)</option><option value="2">Vertical (zigzag U-D)</option></select></label>
          <label id="rotationField">Rotation<select id="rotation"><option value="0">0° (No rotation)</option><option value="1">90° (Clockwise)</option><option value="2">180°</option><option value="3">270° (Counter-CW)</option></select></label>
        </div>
        <h3>Color Correction</h3>
        <div class="form-grid">
          <label>Gamma Correction <span id="gammaValue" style="color:#93c5fd">(2.2)</span><input id="gamma" type="range" min="1.0" max="3.5" step="0.1" style="width:100%"></label>
        </div>
        <h3 id="transformHeader" class="hidden">Transform Options</h3>
        <div class="form-grid" id="transformFields" class="hidden">
          <label class="checkbox"><input id="flipX" type="checkbox"><span>Flip X (Horizontal)</span></label>
          <label class="checkbox"><input id="flipY" type="checkbox"><span>Flip Y (Vertical)</span></label>
          <label class="checkbox"><input id="flipZ" type="checkbox"><span>Flip Z (Diagonal)</span></label>
        </div>
        <button type="submit">Apply Now</button>
      </form>
      <div id="saveMessage">Loading...</div>
      <p style="color:#93c5fd;font-size:.85rem;margin-top:8px">💡 Auto-saves as you change settings</p>
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
        <div><div class="label">Last Frame</div><div class="value" id="lastFrame">-</div></div>
        <div><div class="label">Rendered</div><div class="value" id="renderedFrames">-</div></div>
      </div>
    </section>
  </div>
  <script>
const form=document.getElementById('configForm'),
modeField=document.getElementById('mode'),
sampleModeField=document.getElementById('sampleMode'),
rowIndexField=document.getElementById('rowIndex'),
columnIndexField=document.getElementById('columnIndex'),
linePixelsField=document.getElementById('linePixels'),
rectXField=document.getElementById('rectX'),
rectYField=document.getElementById('rectY'),
rectWidthField=document.getElementById('rectWidth'),
rectHeightField=document.getElementById('rectHeight'),
serpentineField=document.getElementById('serpentine'),
rotationField=document.getElementById('rotation'),
flipXField=document.getElementById('flipX'),
flipYField=document.getElementById('flipY'),
flipZField=document.getElementById('flipZ'),
gammaField=document.getElementById('gamma'),
gammaValue=document.getElementById('gammaValue'),
saveMessage=document.getElementById('saveMessage');
let ws=null,reconnectTimer=null,autoSaveTimer=null;
function setText(id,value){document.getElementById(id).textContent=value}
function updateModeVisibility(){const mode=Number(modeField.value),lineMode=mode===0||mode===1,rowMode=mode===0,columnMode=mode===1,rectangleMode=mode===2;document.getElementById('rowField').classList.toggle('hidden',!rowMode);document.getElementById('columnField').classList.toggle('hidden',!columnMode);document.getElementById('linePixelsField').classList.toggle('hidden',!lineMode);document.getElementById('rectXField').classList.toggle('hidden',!rectangleMode);document.getElementById('rectYField').classList.toggle('hidden',!rectangleMode);document.getElementById('rectWidthField').classList.toggle('hidden',!rectangleMode);document.getElementById('rectHeightField').classList.toggle('hidden',!rectangleMode);document.getElementById('serpentineField').classList.toggle('hidden',!rectangleMode);document.getElementById('rotationField').classList.toggle('hidden',!rectangleMode);document.getElementById('transformHeader').classList.toggle('hidden',!rectangleMode);document.getElementById('transformFields').classList.toggle('hidden',!rectangleMode);sampleModeField.disabled=!lineMode}
function applyConfig(config){modeField.value=config.mode;sampleModeField.value=config.sampleMode;rowIndexField.value=config.rowIndex;columnIndexField.value=config.columnIndex;linePixelsField.value=config.linePixels;linePixelsField.max=config.ledCount;rectXField.value=config.rectX;rectYField.value=config.rectY;rectWidthField.value=config.rectWidth;rectHeightField.value=config.rectHeight;serpentineField.value=config.serpentine||0;rotationField.value=config.rotation||0;flipXField.checked=Boolean(config.flipX);flipYField.checked=Boolean(config.flipY);flipZField.checked=Boolean(config.flipZ);gammaField.value=config.gamma||2.2;gammaValue.textContent='('+gammaField.value+')';updateModeVisibility()}
async function loadConfig(){const response=await fetch('/config.json',{cache:'no-store'});const config=await response.json();applyConfig(config);saveMessage.textContent='Loaded'}
async function saveConfig(showMessage=true){if(showMessage)saveMessage.textContent='Saving...';const payload={mode:Number(modeField.value),sampleMode:Number(sampleModeField.value),rowIndex:Number(rowIndexField.value||0),columnIndex:Number(columnIndexField.value||0),linePixels:Number(linePixelsField.value||1),rectX:Number(rectXField.value||0),rectY:Number(rectYField.value||0),rectWidth:Number(rectWidthField.value||1),rectHeight:Number(rectHeightField.value||1),serpentine:Number(serpentineField.value||0),rotation:Number(rotationField.value||0),flipX:flipXField.checked,flipY:flipYField.checked,flipZ:flipZField.checked,gamma:Number(gammaField.value||2.2)};try{const response=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});if(!response.ok)throw new Error('Save failed');const config=await response.json();applyConfig(config);if(showMessage)saveMessage.textContent='Saved'}catch(error){if(showMessage)saveMessage.textContent=error.message}}
function autoSave(){saveMessage.textContent='Updating...';if(autoSaveTimer)clearTimeout(autoSaveTimer);autoSaveTimer=setTimeout(async()=>{await saveConfig(false);saveMessage.textContent='Updated';setTimeout(()=>{if(saveMessage.textContent==='Updated')saveMessage.textContent=''},2000)},300)}
modeField.addEventListener('change',updateModeVisibility);
modeField.addEventListener('change',autoSave);
sampleModeField.addEventListener('change',autoSave);
rowIndexField.addEventListener('input',autoSave);
columnIndexField.addEventListener('input',autoSave);
linePixelsField.addEventListener('input',autoSave);
rectXField.addEventListener('input',autoSave);
rectYField.addEventListener('input',autoSave);
rectWidthField.addEventListener('input',autoSave);
rectHeightField.addEventListener('input',autoSave);
serpentineField.addEventListener('change',autoSave);
rotationField.addEventListener('change',autoSave);
flipXField.addEventListener('change',autoSave);
flipYField.addEventListener('change',autoSave);
flipZField.addEventListener('change',autoSave);
gammaField.addEventListener('input',()=>{gammaValue.textContent='('+gammaField.value+')';autoSave()});
form.addEventListener('submit',async(event)=>{event.preventDefault();await saveConfig(true)});
function connectWebSocket(){if(ws&&(ws.readyState===WebSocket.CONNECTING||ws.readyState===WebSocket.OPEN))return;const protocol=location.protocol==='https:'?'wss:':'ws:';const wsUrl=`${protocol}//${location.hostname}:81/ws`;try{ws=new WebSocket(wsUrl);ws.onopen=()=>{console.log('WebSocket connected');setText('wsStatus','🟢 Connected');if(reconnectTimer){clearTimeout(reconnectTimer);reconnectTimer=null}};ws.onmessage=(event)=>{try{const status=JSON.parse(event.data);setText('ip',status.ip);setText('wifiStatus',status.wifiStatus);setText('udpPort',status.multicastPort);setText('ledCount',status.ledCount);setText('mapping',status.mapping);setText('lastFrame',status.lastFrame);setText('renderedFrames',status.renderedFrames)}catch(error){console.error('WebSocket message error:',error)}};ws.onerror=(error)=>{console.error('WebSocket error:',error);setText('wsStatus','🔴 Error')};ws.onclose=()=>{console.log('WebSocket disconnected');setText('wsStatus','🟡 Reconnecting...');ws=null;if(!reconnectTimer)reconnectTimer=setTimeout(connectWebSocket,2000)}}catch(error){console.error('Failed to create WebSocket:',error);setText('wsStatus','🔴 Failed');if(!reconnectTimer)reconnectTimer=setTimeout(connectWebSocket,2000)}}
loadConfig();connectWebSocket();
  </script>
</body>
</html>
)HTML";
