/* server.js */
const fs = require('fs');
const path = require('path');
const http = require('http');
const express = require('express');
const { WebSocketServer } = require('ws');
const log = require('./logger');

/* ---- Config ---- */
const GRID_X = parseInt(process.env.GRID_X || '6', 10);
const GRID_Y = parseInt(process.env.GRID_Y || '6', 10);
const Z_LEN  = parseInt(process.env.Z_LEN  || '100', 10);
const PORT   = parseInt(process.env.PORT   || '8080', 10);
const DATA_DIR = path.join(__dirname, 'data');
const MAP_FILE = path.join(DATA_DIR, 'mapping.json');

const VERBOSE_JSON = !!process.env.VERBOSE_JSON;

/* ---- State ---- */
const devices = new Map(); // deviceId -> { ws, id, x, y, zLen, zDir, brightness, lastSeen }
const xyToDevice = new Map(); // "x,y" -> deviceId
let adminClients = new Set();

/* ---- Persisted Mapping ---- */
function ensureDataDir() {
  if (!fs.existsSync(DATA_DIR)) fs.mkdirSync(DATA_DIR);
}
function loadMapping() {
  ensureDataDir();
  if (!fs.existsSync(MAP_FILE)) {
    fs.writeFileSync(MAP_FILE, JSON.stringify({ map: {} }, null, 2));
  }
  const raw = fs.readFileSync(MAP_FILE, 'utf8');
  try {
    const parsed = JSON.parse(raw);
    return parsed.map || {};
  } catch (e) {
    log.error('Mapping', 'Failed to parse mapping.json, starting empty', { error: e.message });
    return {};
  }
}
function saveMapping() {
  ensureDataDir();
  const obj = { map: {} };
  for (const [key, devId] of xyToDevice.entries()) obj.map[key] = devId;
  fs.writeFileSync(MAP_FILE, JSON.stringify(obj, null, 2));
  log.info('Mapping', 'Saved mapping.json', { entries: xyToDevice.size });
}

// Load persisted cell->device mapping
const persisted = loadMapping();
for (const key of Object.keys(persisted)) {
  xyToDevice.set(key, persisted[key]);
}
log.info('Mapping', 'Loaded persisted mapping', { entries: xyToDevice.size });

/* ---- App / HTTP ---- */
const app = express();

// Optional HTTP request logging (simple)
app.use((req, res, next) => {
  const start = Date.now();
  res.on('finish', () => {
    const ms = Date.now() - start;
    log.info('HTTP', `${req.method} ${req.url}`, { status: res.statusCode, ms });
  });
  next();
});

app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

app.get('/api/config', (req, res) => {
  res.json({ gridX: GRID_X, gridY: GRID_Y, zLen: Z_LEN });
});

app.get('/api/devices', (req, res) => {
  const list = Array.from(devices.values()).map(d => ({
    id: d.id, x: d.x ?? null, y: d.y ?? null,
    zLen: d.zLen, zDir: d.zDir, brightness: d.brightness,
    lastSeen: d.lastSeen
  }));
  res.json({
    count: list.length,
    mapped: Array.from(xyToDevice.entries()).map(([k, id]) => ({ cell: k, id })),
    devices: list
  });
});

app.post('/api/assign', (req, res) => {
  const { id, x, y } = req.body || {};
  if (!id || x == null || y == null) {
    log.warn('Admin', 'Assign missing id/x/y', { body: req.body });
    return res.status(400).json({ ok: false, error: 'Missing id/x/y' });
  }
  const d = devices.get(id);
  if (!d) {
    log.warn('Admin', 'Assign device not found', { id, x, y });
    return res.status(404).json({ ok: false, error: 'Device not found' });
  }

  const cellKey = `${x},${y}`;

  // Clear previous occupant of target cell
  if (xyToDevice.has(cellKey)) {
    const prevDevId = xyToDevice.get(cellKey);
    if (prevDevId !== id) {
      const prevDev = devices.get(prevDevId);
      if (prevDev) { prevDev.x = null; prevDev.y = null; }
      log.info('Mapping', 'Reassigning cell (HTTP)', { cell: cellKey, from: prevDevId, to: id });
    }
  }

  // Remove old mapping for this device
  for (const [k, devId] of Array.from(xyToDevice.entries())) {
    if (devId === id) xyToDevice.delete(k);
  }

  d.x = x; d.y = y;
  xyToDevice.set(cellKey, id);
  saveMapping();

  sendJSON(d.ws, { type: 'cfg', x, y });
  broadcastAdmin({ type: 'mapping', mapped: mappingSnapshot(), devices: devicesSnapshot() });
  log.info('Mapping', 'Assigned (HTTP)', { id, cell: cellKey });

  res.json({ ok: true });
});

/* ---- HTTP -> Server ---- */
const server = http.createServer(app);

/* ---- WebSockets ---- */
const wss = new WebSocketServer({ server });

function sendJSON(ws, obj) {
  if (!ws || ws.readyState !== ws.OPEN) return;
  try {
    ws.send(JSON.stringify(obj));
  } catch (e) {
    log.warn('WS', 'Failed to send JSON', { error: e.message });
  }
}

function devicesSnapshot() {
  return Array.from(devices.values()).map(d => ({
    id: d.id, x: d.x ?? null, y: d.y ?? null,
    zLen: d.zLen, zDir: d.zDir, brightness: d.brightness,
    lastSeen: d.lastSeen
  }));
}
function mappingSnapshot() {
  return Array.from(xyToDevice.entries()).map(([cell, id]) => ({ cell, id }));
}
function broadcastAdmin(msg) {
  const json = JSON.stringify(msg);
  for (const ws of adminClients) {
    if (ws.readyState === ws.OPEN) {
      try { ws.send(json); } catch (e) { log.warn('AdminWS', 'Send failed', { error: e.message }); }
    }
  }
}

// Route WS by path: /ws/device vs /ws/admin
wss.on('connection', (ws, req) => {
  const { url, socket } = req;
  ws.isAlive = true;

  if (url.startsWith('/ws/device')) {
    const remote = `${socket.remoteAddress}:${socket.remotePort}`;
    log.info('DeviceWS', 'Incoming connection', { remote });

    ws.on('message', (data) => {
      try {
        const str = (typeof data === 'string') ? data : data.toString('utf8');
        if (VERBOSE_JSON) log.debug('DeviceWS', 'RX', { len: str.length, str });

        const obj = JSON.parse(str);
        if (obj.type === 'hello') {
          const id = obj.id || null;
          if (!id) {
            log.warn('DeviceWS', 'Hello missing id', { remote });
            return;
          }
          const dev = {
            id,
            ws,
            x: null, y: null,
            zLen: obj.zLen ?? Z_LEN,
            zDir: obj.zDir ?? 0,
            brightness: obj.brightness ?? 64,
            lastSeen: Date.now()
          };

          // Attach persisted mapping for this device if any
          for (const [cell, devId] of xyToDevice.entries()) {
            if (devId === id) {
              const [sx, sy] = cell.split(',').map(n => parseInt(n, 10));
              dev.x = sx; dev.y = sy;
              break;
            }
          }

          devices.set(id, dev);
          sendJSON(ws, {
            type: 'hello_ack',
            gridX: GRID_X, gridY: GRID_Y, zLen: dev.zLen,
            assigned: (dev.x != null && dev.y != null) ? { x: dev.x, y: dev.y } : null
          });

          log.info('DeviceWS', 'Device registered', {
            id, zLen: dev.zLen, zDir: dev.zDir, assigned: dev.x != null
          });

          broadcastAdmin({ type: 'devices', devices: devicesSnapshot(), count: devices.size });
          broadcastAdmin({ type: 'mapping', mapped: mappingSnapshot() });

        } else if (obj.type === 'heartbeat') {
          const d = devices.get(obj.id);
          if (d) {
            d.lastSeen = Date.now();
            log.debug('DeviceWS', 'Heartbeat', { id: d.id });
          }
        } else {
          log.debug('DeviceWS', 'Unknown device message', { obj });
        }
      } catch (e) {
        // If message wasn’t JSON, it might be binary later for pixels; ignore
        if (VERBOSE_JSON) log.warn('DeviceWS', 'Non-JSON or parse error', { err: e.message });
      }
    });

    ws.on('close', (code, reasonBuf) => {
      let detachedId = null;
      for (const [id, d] of Array.from(devices.entries())) {
        if (d.ws === ws) {
          devices.delete(id);
          detachedId = id;
          break;
        }
      }
      broadcastAdmin({ type: 'devices', devices: devicesSnapshot(), count: devices.size });
      const reason = reasonBuf?.toString() || '';
      log.info('DeviceWS', 'Closed', { id: detachedId, code, reason });
    });

    ws.on('error', (err) => {
      log.warn('DeviceWS', 'Socket error', { error: err.message });
    });

    ws.on('pong', () => { ws.isAlive = true; });

  } else if (url.startsWith('/ws/admin')) {
    const remote = `${socket.remoteAddress}:${socket.remotePort}`;
    adminClients.add(ws);
    log.info('AdminWS', 'Admin connected', { remote, admins: adminClients.size });

    sendJSON(ws, { type: 'hello_admin', gridX: GRID_X, gridY: GRID_Y, zLen: Z_LEN });
    sendJSON(ws, { type: 'devices', devices: devicesSnapshot(), count: devices.size });
    sendJSON(ws, { type: 'mapping', mapped: mappingSnapshot() });

    ws.on('message', (data) => {
      try {
        const str = (typeof data === 'string') ? data : data.toString('utf8');
        if (VERBOSE_JSON) log.debug('AdminWS', 'RX', { len: str.length, str });
        const obj = JSON.parse(str);
        if (obj.type === 'assign') {
          const { id, x, y } = obj;
          const d = devices.get(id);
          if (!d) {
            log.warn('AdminWS', 'Assign: device not found', { id, x, y });
            return;
          }
          const cellKey = `${x},${y}`;

          // Clear prior occupant
          if (xyToDevice.has(cellKey)) {
            const prevDevId = xyToDevice.get(cellKey);
            if (prevDevId !== id) {
              const prevDev = devices.get(prevDevId);
              if (prevDev) { prevDev.x = null; prevDev.y = null; }
              log.info('Mapping', 'Reassigning cell (WS)', { cell: cellKey, from: prevDevId, to: id });
            }
          }
          // Remove any old mapping for this device
          for (const [k, devId] of Array.from(xyToDevice.entries())) {
            if (devId === id) xyToDevice.delete(k);
          }
          d.x = x; d.y = y;
          xyToDevice.set(cellKey, id);
          saveMapping();
          sendJSON(d.ws, { type: 'cfg', x, y });
          broadcastAdmin({ type: 'mapping', mapped: mappingSnapshot(), devices: devicesSnapshot() });
          log.info('Mapping', 'Assigned (WS)', { id, cell: cellKey });

        } else if (obj.type === 'identify') {
          const { id, ms = 3000 } = obj;
          const d = devices.get(id);
          if (d) {
            sendJSON(d.ws, { type: 'identify', ms });
            log.info('AdminWS', 'Identify', { id, ms });
          } else {
            log.warn('AdminWS', 'Identify: device not found', { id });
          }
        } else {
          log.debug('AdminWS', 'Unknown admin message', { obj });
        }
      } catch (e) {
        log.warn('AdminWS', 'Parse error', { error: e.message });
      }
    });

    ws.on('close', (code, reasonBuf) => {
      adminClients.delete(ws);
      const reason = reasonBuf?.toString() || '';
      log.info('AdminWS', 'Closed', { code, admins: adminClients.size, reason });
    });
    ws.on('error', (err) => {
      log.warn('AdminWS', 'Socket error', { error: err.message });
    });
    ws.on('pong', () => { ws.isAlive = true; });

  } else {
    // Unknown path; treat as admin or close
    adminClients.add(ws);
    log.warn('WS', 'Unknown WS path; added to admin set', { url });
  }
});

/* ---- Ping/Pong keepalive ---- */
setInterval(() => {
  wss.clients.forEach((ws) => {
    if (ws.isAlive === false) {
      log.warn('WS', 'Terminating dead client');
      return ws.terminate();
    }
    ws.isAlive = false;
    try { ws.ping(); } catch (_) {}
  });
}, 15000);

/* ---- Start ---- */
server.listen(PORT, () => {
  log.info('Server', `Listening`, { url: `http://localhost:${PORT}`, gridX: GRID_X, gridY: GRID_Y, zLen: Z_LEN });
  log.info('Server', 'Admin UI', { url: `http://localhost:${PORT}/` });
  log.info('Server', 'Device WS', { url: `ws://localhost:${PORT}/ws/device` });
});