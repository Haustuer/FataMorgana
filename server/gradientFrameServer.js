const express = require("express");
const dgram = require("dgram");
const path = require("path");
const os = require("os");
const sharp = require("sharp");

const app = express();

// Create separate sockets for sending and receiving
const sendSocket = dgram.createSocket({ type: "udp4", reuseAddr: true });
const receiveSocket = dgram.createSocket({ type: "udp4", reuseAddr: true });

const HTTP_PORT = Number(process.env.HTTP_PORT || 3001);
const MULTICAST_ADDR = process.env.MULTICAST_ADDR || "239.255.42.1";
const MULTICAST_PORT = Number(process.env.MULTICAST_PORT || 7777);
const RESPONSE_PORT = Number(process.env.RESPONSE_PORT || 7778);
const MULTICAST_TTL = Number(process.env.MULTICAST_TTL || 1);
const INTER_PACKET_DELAY_MS = Number(process.env.INTER_PACKET_DELAY_MS || 2);
const VERBOSE = process.env.VERBOSE === "true";
const DISCOVERY_TIMEOUT_MS = Number(process.env.DISCOVERY_TIMEOUT_MS || 2000);

// Protocol Constants (see PROJECT_IDEA.md for full specification)
const HEADER_SIZE = 8;
const MAX_PACKET_SIZE = 1200;
const MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
const MAX_FRAME_BYTES = 4800;

// Frame Types
const FRAME_TYPE_CONFIG = 0;            // Config frame
const FRAME_TYPE_IMAGE_START = 1;       // First packet of an image frame
const FRAME_TYPE_IMAGE_CONTINUATION = 2; // Subsequent packets of an image frame

// Config SubTypes
const CONFIG_SUBTYPE_DISCOVERY = 0;     // Discovery request
const CONFIG_SUBTYPE_SET_MAPPING = 1;   // Set device mapping (future)
const CONFIG_SUBTYPE_SET_BRIGHTNESS = 2; // Set brightness (future)
const CONFIG_SUBTYPE_IDENTIFY = 3;      // Identify device (flash LEDs)

// RGB Types (RGB565 uses little-endian byte order)
const RGB332 = 0; // 3 bits red, 3 bits green, 2 bits blue (1 byte per pixel)
const RGB565 = 1; // 5 bits red, 6 bits green, 5 bits blue (2 bytes per pixel, LE)

// Pattern Types
const PATTERN_GRADIENT = "gradient";
const PATTERN_SOLID = "solid";
const PATTERN_CHECKERBOARD = "checkerboard";
const PATTERN_RAINBOW = "rainbow";

const DEFAULT_WIDTH = 24;
const DEFAULT_HEIGHT = 100;
const DEFAULT_RGB_TYPE = RGB332;

let nextFrameCounter = 0;
let lastSendResult = {
  width: DEFAULT_WIDTH,
  height: DEFAULT_HEIGHT,
  rgbType: DEFAULT_RGB_TYPE,
  frameCounter: null,
  chunkCount: 0,
  payloadBytes: 0,
  packetBytes: [],
};

// Store last frame for visualizer
let lastFrameData = {
  width: 0,
  height: 0,
  rgbType: RGB332,
  pixels: null, // Buffer of pixel data
};

// Discovery state
const discoveredDevices = new Map(); // key: IP address, value: device info
let lastDiscoveryTime = null;
let discoveryInProgress = false;

app.use(express.json({ limit: '50mb' }));
app.use(express.static(path.join(__dirname, "gradient-frame-public")));

// Get server IP address
function getServerIP() {
  const interfaces = os.networkInterfaces();
  for (const name of Object.keys(interfaces)) {
    for (const iface of interfaces[name]) {
      // Skip internal and non-IPv4 addresses
      if (iface.family === 'IPv4' && !iface.internal) {
        return iface.address;
      }
    }
  }
  return '127.0.0.1'; // Fallback
}

const SERVER_IP = getServerIP();

// Setup send socket for multicast
sendSocket.bind(() => {
  sendSocket.setMulticastTTL(MULTICAST_TTL);
  log("Send socket ready", {
    multicastAddr: MULTICAST_ADDR,
    multicastPort: MULTICAST_PORT,
    ttl: MULTICAST_TTL
  });
});

// Setup receive socket for unicast responses
receiveSocket.bind(RESPONSE_PORT, () => {
  log("Receive socket ready", {
    responsePort: RESPONSE_PORT,
    serverIP: SERVER_IP
  });
});

// Parse binary discovery response
function parseDiscoveryResponse(buffer) {
  if (buffer.length !== 69) {
    throw new Error(`Invalid discovery response length: ${buffer.length} (expected 69)`);
  }

  // Check magic bytes "FATA" (0x46415441)
  const magic = buffer.readUInt32BE(0);
  if (magic !== 0x46415441) {
    throw new Error(`Invalid magic bytes: 0x${magic.toString(16)}`);
  }

  const version = buffer.readUInt8(4);
  const type = buffer.readUInt8(5);

  if (type !== 0x01) {
    throw new Error(`Invalid response type: ${type}`);
  }

  // Parse IP address (network byte order - big-endian)
  const ip = `${buffer.readUInt8(8)}.${buffer.readUInt8(9)}.${buffer.readUInt8(10)}.${buffer.readUInt8(11)}`;

  // Parse MAC address
  const mac = Array.from(buffer.subarray(12, 18))
    .map(b => b.toString(16).padStart(2, '0').toUpperCase())
    .join(':');

  const chipId = buffer.readUInt32LE(18);
  const uptime = buffer.readUInt32LE(22);

  const ledCount = buffer.readUInt16LE(26);
  const ledPin = buffer.readUInt8(28);
  const brightness = buffer.readUInt8(29);
  const firmwareMajor = buffer.readUInt8(30);
  const firmwareMinor = buffer.readUInt8(31);

  const mappingMode = buffer.readUInt8(32);
  const sampleMode = buffer.readUInt8(33);
  const rowColumnIndex = buffer.readUInt16LE(34);
  const linePixels = buffer.readUInt16LE(36);
  const rectX = buffer.readUInt16LE(38);
  const rectY = buffer.readUInt16LE(40);
  const rectWidth = buffer.readUInt16LE(42);
  const rectHeight = buffer.readUInt16LE(44);
  const serpentine = buffer.readUInt8(46);  // 0=none, 1=horizontal, 2=vertical

  // Parse transform byte (position 47)
  const transformByte = buffer.readUInt8(47);
  const rotation = transformByte & 0x03;           // Bits 0-1
  const flipX = (transformByte & 0x04) !== 0;      // Bit 2
  const flipY = (transformByte & 0x08) !== 0;      // Bit 3
  const flipZ = (transformByte & 0x10) !== 0;      // Bit 4

  const acceptedPackets = buffer.readUInt32LE(48);
  const rejectedPackets = buffer.readUInt32LE(52);
  const renderedFrames = buffer.readUInt32LE(56);
  const lastFrameWidth = buffer.readUInt16LE(60);
  const lastFrameHeight = buffer.readUInt16LE(62);

  // Gamma correction (float, little-endian)
  const gamma = buffer.readFloatLE(64);

  // Out-of-bounds mode
  const oobMode = buffer.readUInt8(68);

  const modeNames = ['row', 'column', 'rectangle'];
  const sampleModeNames = ['pixel', 'interpolated'];
  const serpentineModeNames = ['none', 'horizontal', 'vertical'];
  const oobModeNames = ['black', 'clamp', 'mirror'];

  return {
    protocol: 'FataMorgana',
    version,
    type: 'discovery_response',
    device: {
      ip,
      mac,
      chipId: chipId.toString(16).toUpperCase().padStart(8, '0'),
      hostname: `ESP-${chipId.toString(16).toUpperCase().substring(0, 6)}`,
      firmware: `${firmwareMajor}.${firmwareMinor}`,
      uptime
    },
    hardware: {
      ledCount,
      ledPin,
      brightness,
      ledType: 'WS2812B'
    },
    mapping: {
      mode: mappingMode,
      modeName: modeNames[mappingMode] || 'unknown',
      sampleMode,
      sampleModeName: sampleModeNames[sampleMode] || 'unknown',
      rowIndex: rowColumnIndex,
      columnIndex: rowColumnIndex,
      linePixels,
      rectX,
      rectY,
      rectWidth,
      rectHeight,
      serpentine,
      serpentineModeName: serpentineModeNames[serpentine] || 'unknown',
      rotation,
      flipX,
      flipY,
      flipZ,
      gamma,
      oobMode,
      oobModeName: oobModeNames[oobMode] || 'unknown'
    },
    status: {
      lastFrameCounter: 0,
      lastFrameWidth,
      lastFrameHeight,
      lastFrameRgbType: 0,
      lastFrameMillis: 0,
      acceptedPackets,
      rejectedPackets,
      renderedFrames
    }
  };
}

// Handle incoming discovery responses
receiveSocket.on("message", (msg, rinfo) => {
  try {
    // Try to parse as binary discovery response
    const response = parseDiscoveryResponse(msg);

    const deviceIP = response.device.ip;
    discoveredDevices.set(deviceIP, {
      ...response,
      receivedAt: Date.now(),
      remoteAddress: rinfo.address // Actual source IP
    });

    log("Discovery response received", {
      ip: deviceIP,
      mac: response.device.mac,
      hostname: response.device.hostname,
      chipId: response.device.chipId,
      mapping: response.mapping.modeName,
      leds: response.hardware.ledCount
    });
  } catch (error) {
    logVerbose("Failed to parse discovery response", {
      error: error.message,
      from: rinfo.address,
      length: msg.length
    });
  }
});

receiveSocket.on("error", (err) => {
  log("Receive socket error", { error: err.message });
});

// Logging utilities
function log(message, data = {}) {
  const timestamp = new Date().toISOString();
  console.log(`[${timestamp}] ${message}`, data);
}

function logVerbose(message, data = {}) {
  if (VERBOSE) {
    log(message, data);
  }
}

// Utility to add delay between packets
function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function normalizeDimension(value, fallback) {
  const number = Number(value);

  if (!Number.isFinite(number)) {
    return fallback;
  }

  return Math.max(1, Math.min(65535, Math.round(number)));
}

function normalizeRgbType(value) {
  return Number(value) === RGB565 ? RGB565 : RGB332;
}

function rgbTypeName(rgbType) {
  return rgbType === RGB565 ? "RGB565" : "RGB332";
}

function bytesPerPixel(rgbType) {
  return rgbType === RGB565 ? 2 : 1;
}

function encodeRgb332Pixel(r, g, b) {
  return (r & 0xe0) | ((g & 0xe0) >> 3) | (b >> 6);
}

function encodeRgb565Pixel(r, g, b) {
  return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
}

// Pattern Generators
function buildGradientFrame(width, height, rgbType) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = y * width + x;
      const r = Math.round((y / Math.max(1, height - 1)) * 255);
      const g = Math.round((x / Math.max(1, width - 1)) * 255);
      const b = Math.round(((x + y) / Math.max(1, width + height - 2)) * 255);

      if (rgbType === RGB565) {
        payload.writeUInt16LE(encodeRgb565Pixel(r, g, b), pixelIndex * 2);
      } else {
        payload[pixelIndex] = encodeRgb332Pixel(r, g, b);
      }
    }
  }

  return payload;
}

function buildSolidFrame(width, height, rgbType, r = 255, g = 0, b = 0) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));
  const pixelCount = width * height;

  for (let i = 0; i < pixelCount; i += 1) {
    if (rgbType === RGB565) {
      payload.writeUInt16LE(encodeRgb565Pixel(r, g, b), i * 2);
    } else {
      payload[i] = encodeRgb332Pixel(r, g, b);
    }
  }

  return payload;
}

function buildCheckerboardFrame(width, height, rgbType, cellSize = 4) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = y * width + x;
      const isWhite = (Math.floor(x / cellSize) + Math.floor(y / cellSize)) % 2 === 0;
      const color = isWhite ? 255 : 0;

      if (rgbType === RGB565) {
        payload.writeUInt16LE(encodeRgb565Pixel(color, color, color), pixelIndex * 2);
      } else {
        payload[pixelIndex] = encodeRgb332Pixel(color, color, color);
      }
    }
  }

  return payload;
}

function buildRainbowFrame(width, height, rgbType, offset = 0) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = y * width + x;
      // Create rainbow based on position
      const hue = ((x + y + offset) % (width + height)) / (width + height);
      const { r, g, b } = hsvToRgb(hue, 1.0, 1.0);

      if (rgbType === RGB565) {
        payload.writeUInt16LE(encodeRgb565Pixel(r, g, b), pixelIndex * 2);
      } else {
        payload[pixelIndex] = encodeRgb332Pixel(r, g, b);
      }
    }
  }

  return payload;
}

// HSV to RGB conversion for rainbow pattern
function hsvToRgb(h, s, v) {
  const i = Math.floor(h * 6);
  const f = h * 6 - i;
  const p = v * (1 - s);
  const q = v * (1 - f * s);
  const t = v * (1 - (1 - f) * s);

  let r, g, b;
  switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }

  return {
    r: Math.round(r * 255),
    g: Math.round(g * 255),
    b: Math.round(b * 255),
  };
}

// Split frame payload into UDP packets with proper headers
function splitIntoPackets(width, height, rgbType, frameCounter, payload) {
  const packets = [];
  const chunkCount = Math.max(1, Math.ceil(payload.length / MAX_PAYLOAD_SIZE));

  for (let chunkIndex = 0; chunkIndex < chunkCount; chunkIndex += 1) {
    const offset = chunkIndex * MAX_PAYLOAD_SIZE;
    const chunk = payload.subarray(offset, offset + MAX_PAYLOAD_SIZE);
    const packet = Buffer.alloc(HEADER_SIZE + chunk.length);

    // Build 8-byte header:
    // Byte 0: Frame Type
    // Byte 1: Frame Counter
    // Byte 2: Chunk Index
    // Byte 3: RGB Type
    // Bytes 4-5: Width (little-endian)
    // Bytes 6-7: Height (little-endian)
    packet[0] = chunkIndex === 0 ? FRAME_TYPE_IMAGE_START : FRAME_TYPE_IMAGE_CONTINUATION;
    packet[1] = frameCounter;
    packet[2] = chunkIndex;
    packet[3] = rgbType;
    packet.writeUInt16LE(width, 4);
    packet.writeUInt16LE(height, 6);
    chunk.copy(packet, HEADER_SIZE);

    logVerbose(`Packet ${chunkIndex + 1}/${chunkCount}`, {
      frameType: packet[0],
      frameCounter,
      chunkIndex,
      payloadBytes: chunk.length,
      totalBytes: packet.length,
    });

    packets.push(packet);
  }

  return { chunkCount, packets };
}

function sendUdpPacket(packet) {
  return new Promise((resolve, reject) => {
    sendSocket.send(packet, 0, packet.length, MULTICAST_PORT, MULTICAST_ADDR, (error) => {
      if (error) {
        reject(error);
        return;
      }

      resolve();
    });
  });
}

// Send a frame with the specified pattern
async function sendFrame({ width, height, rgbType, pattern = PATTERN_GRADIENT, patternOptions = {} }) {
  const startTime = Date.now();

  // Build payload based on pattern type
  let payload;
  switch (pattern) {
    case PATTERN_SOLID:
      payload = buildSolidFrame(
        width,
        height,
        rgbType,
        patternOptions.r || 255,
        patternOptions.g || 0,
        patternOptions.b || 0
      );
      break;
    case PATTERN_CHECKERBOARD:
      payload = buildCheckerboardFrame(width, height, rgbType, patternOptions.cellSize || 4);
      break;
    case PATTERN_RAINBOW:
      payload = buildRainbowFrame(width, height, rgbType, patternOptions.offset || 0);
      break;
    case PATTERN_GRADIENT:
    default:
      payload = buildGradientFrame(width, height, rgbType);
      break;
  }

  const frameCounter = nextFrameCounter;
  const { chunkCount, packets } = splitIntoPackets(width, height, rgbType, frameCounter, payload);

  nextFrameCounter = (nextFrameCounter + 1) & 0xff;

  log(`Sending frame #${frameCounter}`, {
    pattern,
    dimensions: `${width}x${height}`,
    rgbType: rgbTypeName(rgbType),
    chunks: chunkCount,
    payloadBytes: payload.length,
  });

  // Send packets with delay to prevent overwhelming ESP WiFi buffer
  for (let i = 0; i < packets.length; i++) {
    await sendUdpPacket(packets[i]);

    // Add delay between packets (except after the last one)
    if (i < packets.length - 1 && INTER_PACKET_DELAY_MS > 0) {
      await delay(INTER_PACKET_DELAY_MS);
    }
  }

  const elapsed = Date.now() - startTime;

  lastSendResult = {
    width,
    height,
    rgbType,
    pattern,
    frameCounter,
    chunkCount,
    payloadBytes: payload.length,
    packetBytes: packets.map((packet) => packet.length),
    elapsedMs: elapsed,
  };

  // Store frame data for visualizer
  lastFrameData = {
    width,
    height,
    rgbType,
    pixels: payload,
  };

  log(`Frame #${frameCounter} sent`, { elapsedMs: elapsed });

  return lastSendResult;
}

// Send discovery request to all devices
async function sendDiscoveryRequest() {
  if (discoveryInProgress) {
    log("Discovery already in progress");
    return { ok: false, message: "Discovery already in progress" };
  }

  discoveryInProgress = true;
  discoveredDevices.clear();

  const packet = Buffer.alloc(14); // 8 byte header + 6 byte payload

  // Header
  packet[0] = FRAME_TYPE_CONFIG;           // Type = 0
  packet[1] = nextFrameCounter;            // Frame counter
  packet[2] = 0;                           // Chunk index = 0
  packet[3] = CONFIG_SUBTYPE_DISCOVERY;    // SubType = 0 (discovery)
  packet.writeUInt16LE(0, 4);              // Reserved
  packet.writeUInt16LE(0, 6);              // Reserved

  // Payload: Server IP (big-endian) + Response port (big-endian)
  const ipParts = SERVER_IP.split('.').map(Number);
  packet[8] = ipParts[0];
  packet[9] = ipParts[1];
  packet[10] = ipParts[2];
  packet[11] = ipParts[3];
  packet.writeUInt16BE(RESPONSE_PORT, 12); // Port in big-endian

  nextFrameCounter = (nextFrameCounter + 1) & 0xff;

  log("Sending discovery request", {
    serverIP: SERVER_IP,
    responsePort: RESPONSE_PORT,
    multicastAddr: MULTICAST_ADDR
  });

  try {
    await sendUdpPacket(packet);
    lastDiscoveryTime = Date.now();

    // Wait for responses
    await delay(DISCOVERY_TIMEOUT_MS);

    discoveryInProgress = false;

    log("Discovery complete", {
      devicesFound: discoveredDevices.size,
      timeout: DISCOVERY_TIMEOUT_MS
    });

    return {
      ok: true,
      devicesFound: discoveredDevices.size,
      devices: Array.from(discoveredDevices.values())
    };
  } catch (error) {
    discoveryInProgress = false;
    log("Discovery failed", { error: error.message });
    return { ok: false, error: error.message };
  }
}

// Send identify request to a specific device
async function sendIdentifyRequest({ targetType, targetValue, flashCount = 3 }) {
  // targetType: 0 = IP address, 1 = Chip ID
  // targetValue: IP address string or chip ID number
  // flashCount: number of times to flash (1-255)

  const packet = Buffer.alloc(14); // 8 byte header + 6 byte payload

  // Header
  packet[0] = FRAME_TYPE_CONFIG;           // Type = 0
  packet[1] = nextFrameCounter;            // Frame counter
  packet[2] = 0;                           // Chunk index = 0
  packet[3] = CONFIG_SUBTYPE_IDENTIFY;     // SubType = 3 (identify)
  packet.writeUInt16LE(0, 4);              // Reserved
  packet.writeUInt16LE(0, 6);              // Reserved

  // Payload: Target type + Target value + Flash count
  packet[8] = targetType; // 0 = IP, 1 = Chip ID

  if (targetType === 0) {
    // IP address (big-endian)
    const ipParts = targetValue.split('.').map(Number);
    packet[9] = ipParts[0];
    packet[10] = ipParts[1];
    packet[11] = ipParts[2];
    packet[12] = ipParts[3];
  } else {
    // Chip ID (little-endian)
    const chipId = typeof targetValue === 'string' ? parseInt(targetValue, 16) : targetValue;
    packet.writeUInt32LE(chipId, 9);
  }

  packet[13] = Math.max(1, Math.min(255, flashCount)); // Flash count (1-255)

  nextFrameCounter = (nextFrameCounter + 1) & 0xff;

  log("Sending identify request", {
    targetType: targetType === 0 ? 'IP' : 'Chip ID',
    targetValue,
    flashCount: packet[13]
  });

  try {
    await sendUdpPacket(packet);
    return {
      ok: true,
      targetType: targetType === 0 ? 'IP' : 'Chip ID',
      targetValue,
      flashCount: packet[13]
    };
  } catch (error) {
    log("Identify request failed", { error: error.message });
    return { ok: false, error: error.message };
  }
}

// Calculate coverage for a device
function calculateCoverage(device, imageWidth, imageHeight) {
  const { mapping } = device;

  if (mapping.mode === 0) { // ROW
    return {
      mode: "row",
      x: 0,
      y: mapping.rowIndex,
      width: Math.min(mapping.linePixels, imageWidth),
      height: 1,
      pixels: Math.min(mapping.linePixels, imageWidth)
    };
  }

  if (mapping.mode === 1) { // COLUMN
    return {
      mode: "column",
      x: mapping.columnIndex,
      y: 0,
      width: 1,
      height: Math.min(mapping.linePixels, imageHeight),
      pixels: Math.min(mapping.linePixels, imageHeight)
    };
  }

  if (mapping.mode === 2) { // RECTANGLE
    return {
      mode: "rectangle",
      x: mapping.rectX,
      y: mapping.rectY,
      width: mapping.rectWidth,
      height: mapping.rectHeight,
      pixels: mapping.rectWidth * mapping.rectHeight,
      serpentine: mapping.serpentine
    };
  }

  return null;
}

function buildSettingsResponse() {
  return {
    ok: true,
    httpPort: HTTP_PORT,
    multicastAddr: MULTICAST_ADDR,
    multicastPort: MULTICAST_PORT,
    responsePort: RESPONSE_PORT,
    serverIP: SERVER_IP,
    interPacketDelayMs: INTER_PACKET_DELAY_MS,
    verbose: VERBOSE,
    defaults: {
      width: DEFAULT_WIDTH,
      height: DEFAULT_HEIGHT,
      rgbType: DEFAULT_RGB_TYPE,
      pattern: PATTERN_GRADIENT,
    },
    limits: {
      maxFrameBytes: MAX_FRAME_BYTES,
      maxPayloadBytesPerPacket: MAX_PAYLOAD_SIZE,
      headerSize: HEADER_SIZE,
      maxPacketSize: MAX_PACKET_SIZE,
    },
    rgbTypes: [
      { value: RGB332, label: "RGB332", bytesPerPixel: 1 },
      { value: RGB565, label: "RGB565", bytesPerPixel: 2 },
    ],
    patterns: [
      { value: PATTERN_GRADIENT, label: "Gradient" },
      { value: PATTERN_SOLID, label: "Solid Color" },
      { value: PATTERN_CHECKERBOARD, label: "Checkerboard" },
      { value: PATTERN_RAINBOW, label: "Rainbow" },
    ],
    lastSend: lastSendResult.frameCounter !== null ? {
      ...lastSendResult,
      rgbTypeLabel: rgbTypeName(lastSendResult.rgbType),
    } : null,
  };
}

app.get("/health", (_req, res) => {
  res.json(buildSettingsResponse());
});

app.get("/api/config", (_req, res) => {
  res.json(buildSettingsResponse());
});

app.post("/api/frame", async (req, res) => {
  const width = normalizeDimension(req.body.width, DEFAULT_WIDTH);
  const height = normalizeDimension(req.body.height, DEFAULT_HEIGHT);
  const rgbType = normalizeRgbType(req.body.rgbType);
  const pattern = req.body.pattern || PATTERN_GRADIENT;
  const patternOptions = req.body.patternOptions || {};

  const payloadBytes = width * height * bytesPerPixel(rgbType);

  if (payloadBytes > MAX_FRAME_BYTES) {
    res.status(400).json({
      ok: false,
      error: `Frame too large for firmware buffer (${payloadBytes} > ${MAX_FRAME_BYTES} bytes)`,
      payloadBytes,
      maxFrameBytes: MAX_FRAME_BYTES,
    });
    return;
  }

  try {
    const result = await sendFrame({ width, height, rgbType, pattern, patternOptions });
    res.json({
      ok: true,
      ...result,
      rgbTypeLabel: rgbTypeName(rgbType),
    });
  } catch (error) {
    log("Error sending frame", { error: error.message });
    res.status(500).json({ ok: false, error: error.message });
  }
});

app.post("/api/frame/image", async (req, res) => {
  try {
    if (!req.body) {
      res.status(400).json({ ok: false, error: 'No request body received' });
      return;
    }

    const { image, rgbType = RGB332, targetWidth, targetHeight } = req.body;

    if (!image) {
      res.status(400).json({ ok: false, error: 'No image data provided in request body' });
      return;
    }

    if (typeof image !== 'string') {
      res.status(400).json({ ok: false, error: 'Image data must be a base64 string' });
      return;
    }

    if (!image.startsWith('data:image/')) {
      res.status(400).json({ ok: false, error: 'Image must be in data URL format (data:image/...)' });
      return;
    }

    // Parse base64 image data
    const base64Data = image.replace(/^data:image\/\w+;base64,/, '');

    if (base64Data.length === 0) {
      res.status(400).json({ ok: false, error: 'Image data is empty' });
      return;
    }

    log("Processing image upload", {
      dataUrlLength: image.length,
      base64Length: base64Data.length,
      targetWidth,
      targetHeight,
      rgbType: rgbTypeName(rgbType)
    });

    let buffer;
    try {
      buffer = Buffer.from(base64Data, 'base64');
    } catch (error) {
      res.status(400).json({ ok: false, error: 'Failed to decode base64 image data: ' + error.message });
      return;
    }

    // Load image with sharp
    let sharpImage;
    try {
      sharpImage = sharp(buffer);
    } catch (error) {
      res.status(400).json({ ok: false, error: 'Failed to load image: ' + error.message });
      return;
    }

    // Resize if target dimensions provided
    if (targetWidth && targetHeight) {
      try {
        sharpImage = sharpImage.resize(targetWidth, targetHeight, {
          fit: 'fill',
          kernel: 'lanczos3'  // High-quality downsampling
        });
      } catch (error) {
        res.status(400).json({ ok: false, error: 'Failed to resize image: ' + error.message });
        return;
      }
    }

    let metadata;
    try {
      metadata = await sharpImage.metadata();
    } catch (error) {
      res.status(400).json({ ok: false, error: 'Failed to read image metadata: ' + error.message });
      return;
    }

    const width = targetWidth || metadata.width;
    const height = targetHeight || metadata.height;

    const payloadBytes = width * height * bytesPerPixel(rgbType);

    if (payloadBytes > MAX_FRAME_BYTES) {
      res.status(400).json({
        ok: false,
        error: `Image too large for firmware buffer (${payloadBytes} > ${MAX_FRAME_BYTES} bytes). Resize to max ${Math.floor(MAX_FRAME_BYTES / bytesPerPixel(rgbType))} pixels.`,
        payloadBytes,
        maxFrameBytes: MAX_FRAME_BYTES,
      });
      return;
    }

    // Get raw RGBA pixel data
    let rgbaBuffer;
    try {
      const result = await sharpImage
        .raw()
        .ensureAlpha()
        .toBuffer({ resolveWithObject: true });
      rgbaBuffer = result.data;
    } catch (error) {
      res.status(500).json({ ok: false, error: 'Failed to extract pixel data: ' + error.message });
      return;
    }

    // Convert RGBA to target format
    const pixelBuffer = Buffer.alloc(payloadBytes);
    let bufferIndex = 0;

    for (let i = 0; i < rgbaBuffer.length; i += 4) {
      const r = rgbaBuffer[i];
      const g = rgbaBuffer[i + 1];
      const b = rgbaBuffer[i + 2];
      // const a = rgbaBuffer[i + 3]; // alpha channel (unused for now)

      if (rgbType === RGB332) {
        // 3 bits red, 3 bits green, 2 bits blue
        const r3 = Math.floor((r / 255) * 7);
        const g3 = Math.floor((g / 255) * 7);
        const b2 = Math.floor((b / 255) * 3);
        pixelBuffer[bufferIndex++] = (r3 << 5) | (g3 << 2) | b2;
      } else {
        // RGB565: 5 bits red, 6 bits green, 5 bits blue (little-endian)
        const r5 = Math.floor((r / 255) * 31);
        const g6 = Math.floor((g / 255) * 63);
        const b5 = Math.floor((b / 255) * 31);
        const rgb565 = (r5 << 11) | (g6 << 5) | b5;
        pixelBuffer[bufferIndex++] = rgb565 & 0xFF; // Low byte
        pixelBuffer[bufferIndex++] = (rgb565 >> 8) & 0xFF; // High byte
      }
    }

    // Send the frame
    const startTime = Date.now();
    const chunkCount = Math.ceil(payloadBytes / MAX_PAYLOAD_SIZE);
    const frameCounter = nextFrameCounter++;

    for (let chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {
      const offset = chunkIndex * MAX_PAYLOAD_SIZE;
      const remaining = payloadBytes - offset;
      const chunkSize = remaining < MAX_PAYLOAD_SIZE ? remaining : MAX_PAYLOAD_SIZE;
      const payload = pixelBuffer.subarray(offset, offset + chunkSize);

      const frameType = chunkIndex === 0 ? FRAME_TYPE_IMAGE_START : FRAME_TYPE_IMAGE_CONTINUATION;
      const header = Buffer.alloc(HEADER_SIZE);
      header[0] = frameType;
      header[1] = frameCounter;
      header[2] = chunkIndex;
      header[3] = rgbType;
      header.writeUInt16LE(width, 4);
      header.writeUInt16LE(height, 6);

      const packet = Buffer.concat([header, payload]);
      await sendUdpPacket(packet);

      if (chunkIndex < chunkCount - 1 && INTER_PACKET_DELAY_MS > 0) {
        await delay(INTER_PACKET_DELAY_MS);
      }
    }

    const elapsedMs = Date.now() - startTime;

    // Store frame data for visualizer
    lastFrameData = {
      width,
      height,
      rgbType,
      pixels: pixelBuffer,
    };

    log("Image frame sent", {
      frameCounter,
      width,
      height,
      resized: targetWidth && targetHeight ? `${targetWidth}×${targetHeight}` : 'no',
      rgbType: rgbTypeName(rgbType),
      chunkCount,
      payloadBytes,
      elapsedMs
    });

    res.json({
      ok: true,
      frameCounter,
      width,
      height,
      rgbType,
      chunkCount,
      payloadBytes,
      elapsedMs
    });

  } catch (error) {
    log("Error sending image", {
      error: error.message,
      stack: error.stack,
      hasBody: !!req.body,
      bodyKeys: req.body ? Object.keys(req.body) : []
    });
    res.status(500).json({ ok: false, error: error.message || 'Unknown error processing image' });
  }
});

app.post("/api/discover", async (_req, res) => {
  try {
    const result = await sendDiscoveryRequest();
    res.json(result);
  } catch (error) {
    log("Discovery error", { error: error.message });
    res.status(500).json({ ok: false, error: error.message });
  }
});

app.post("/api/identify", async (req, res) => {
  try {
    const { ip, chipId, flashCount = 3 } = req.body;

    if (!ip && !chipId) {
      res.status(400).json({
        ok: false,
        error: "Must provide either 'ip' or 'chipId' parameter"
      });
      return;
    }

    if (ip && chipId) {
      res.status(400).json({
        ok: false,
        error: "Provide only one of 'ip' or 'chipId', not both"
      });
      return;
    }

    const targetType = ip ? 0 : 1;
    const targetValue = ip || chipId;

    const result = await sendIdentifyRequest({
      targetType,
      targetValue,
      flashCount
    });

    res.json(result);
  } catch (error) {
    log("Identify error", { error: error.message });
    res.status(500).json({ ok: false, error: error.message });
  }
});

app.get("/api/devices", (_req, res) => {
  const devices = Array.from(discoveredDevices.values()).map(device => ({
    ip: device.device.ip,
    mac: device.device.mac,
    hostname: device.device.hostname,
    firmware: device.device.firmware,
    uptime: device.device.uptime,
    ledCount: device.hardware.ledCount,
    brightness: device.hardware.brightness,
    mapping: device.mapping,
    lastFrame: device.status,
    receivedAt: device.receivedAt
  }));

  res.json({
    ok: true,
    serverIP: SERVER_IP,
    lastDiscovery: lastDiscoveryTime,
    deviceCount: devices.length,
    devices
  });
});

app.get("/api/frame/last", (_req, res) => {
  if (!lastFrameData.pixels) {
    res.json({
      ok: false,
      message: 'No frame data available'
    });
    return;
  }

  // Convert pixel buffer to base64 for transmission
  res.json({
    ok: true,
    width: lastFrameData.width,
    height: lastFrameData.height,
    rgbType: lastFrameData.rgbType,
    pixels: lastFrameData.pixels.toString('base64')
  });
});

app.get("/api/coverage", (req, res) => {
  const width = normalizeDimension(req.query.width, DEFAULT_WIDTH);
  const height = normalizeDimension(req.query.height, DEFAULT_HEIGHT);

  const totalPixels = width * height;
  const deviceCoverage = [];
  let coveredPixels = 0;

  for (const device of discoveredDevices.values()) {
    const coverage = calculateCoverage(device, width, height);
    if (coverage) {
      deviceCoverage.push({
        ip: device.device.ip,
        hostname: device.device.hostname,
        ...coverage
      });
      coveredPixels += coverage.pixels;
    }
  }

  res.json({
    ok: true,
    imageSize: { width, height },
    totalPixels,
    coveredPixels,
    coveragePercent: totalPixels > 0 ? (coveredPixels / totalPixels * 100).toFixed(2) : 0,
    devices: deviceCoverage
  });
});

app.listen(HTTP_PORT, () => {
  console.log("\n╔══════════════════════════════════════════════════════════╗");
  console.log("║         FataMorgana Gradient Frame Server v1.0           ║");
  console.log("╠══════════════════════════════════════════════════════════╣");
  console.log(`║ HTTP Server:       http://localhost:${HTTP_PORT}                  ║`);
  console.log(`║ Server IP:         ${SERVER_IP.padEnd(39)}║`);
  console.log(`║ Multicast Group:   ${MULTICAST_ADDR}:${MULTICAST_PORT}${' '.repeat(27)}║`);
  console.log(`║ Response Port:     ${RESPONSE_PORT}${' '.repeat(40)}║`);
  console.log(`║ Multicast TTL:     ${MULTICAST_TTL}${' '.repeat(40)}║`);
  console.log(`║ Inter-packet delay: ${INTER_PACKET_DELAY_MS}ms${' '.repeat(38)}║`);
  console.log(`║ Verbose logging:   ${VERBOSE ? 'enabled' : 'disabled'}${' '.repeat(32)}║`);
  console.log("╠══════════════════════════════════════════════════════════╣");
  console.log("║ Available Patterns:                                      ║");
  console.log(`║   • ${PATTERN_GRADIENT}, ${PATTERN_SOLID}, ${PATTERN_CHECKERBOARD}, ${PATTERN_RAINBOW}${' '.repeat(9)}║`);
  console.log("╠══════════════════════════════════════════════════════════╣");
  console.log("║ API Endpoints:                                           ║");
  console.log("║   GET  /api/config    - Server configuration             ║");
  console.log("║   POST /api/frame     - Send image frame                 ║");
  console.log("║   POST /api/discover  - Discover devices                 ║");
  console.log("║   POST /api/identify  - Identify device (flash LEDs)     ║");
  console.log("║   GET  /api/devices   - List discovered devices          ║");
  console.log("║   GET  /api/coverage  - Calculate image coverage         ║");
  console.log("╠══════════════════════════════════════════════════════════╣");
  console.log("║ Examples:                                                ║");
  console.log("║                                                          ║");
  console.log("║ # Discover devices                                       ║");
  console.log(`║ curl -X POST http://localhost:${HTTP_PORT}/api/discover           ║`);
  console.log("║                                                          ║");
  console.log("║ # Identify device by IP (flash 5 times)                 ║");
  console.log(`║ curl -X POST http://localhost:${HTTP_PORT}/api/identify \\        ║`);
  console.log("║   -H 'Content-Type: application/json' \\                 ║");
  console.log("║   -d '{\"ip\":\"192.168.1.100\",\"flashCount\":5}'          ║");
  console.log("║                                                          ║");
  console.log("║ # Send gradient frame                                    ║");
  console.log(`║ curl -X POST http://localhost:${HTTP_PORT}/api/frame \\          ║`);
  console.log("║   -H 'Content-Type: application/json' \\                 ║");
  console.log("║   -d '{\"width\":24,\"height\":100,\"rgbType\":0}'           ║");
  console.log("╚══════════════════════════════════════════════════════════╝\n");
});
