const express = require("express");
const dgram = require("dgram");
const path = require("path");

const app = express();
const udp = dgram.createSocket("udp4");

const HTTP_PORT = Number(process.env.HTTP_PORT || 3001);
const UDP_PORT = Number(process.env.UDP_PORT || 7777);
const BROADCAST_ADDR = process.env.BROADCAST_ADDR || "255.255.255.255";

const HEADER_SIZE = 8;
const MAX_PACKET_SIZE = 1200;
const MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
const MAX_FRAME_BYTES = 4800;

const FRAME_TYPE_IMAGE_START = 1;
const FRAME_TYPE_IMAGE_CONTINUATION = 2;

const RGB332 = 0;
const RGB565 = 1;

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

app.use(express.json());
app.use(express.static(path.join(__dirname, "gradient-frame-public")));

udp.bind(() => {
  udp.setBroadcast(true);
});

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

function splitIntoPackets(width, height, rgbType, frameCounter, payload) {
  const packets = [];
  const chunkCount = Math.max(1, Math.ceil(payload.length / MAX_PAYLOAD_SIZE));

  for (let chunkIndex = 0; chunkIndex < chunkCount; chunkIndex += 1) {
    const offset = chunkIndex * MAX_PAYLOAD_SIZE;
    const chunk = payload.subarray(offset, offset + MAX_PAYLOAD_SIZE);
    const packet = Buffer.alloc(HEADER_SIZE + chunk.length);

    packet[0] = chunkIndex === 0 ? FRAME_TYPE_IMAGE_START : FRAME_TYPE_IMAGE_CONTINUATION;
    packet[1] = frameCounter;
    packet[2] = chunkIndex;
    packet[3] = rgbType;
    packet.writeUInt16LE(width, 4);
    packet.writeUInt16LE(height, 6);
    chunk.copy(packet, HEADER_SIZE);

    packets.push(packet);
  }

  return { chunkCount, packets };
}

function sendUdpPacket(packet) {
  return new Promise((resolve, reject) => {
    udp.send(packet, 0, packet.length, UDP_PORT, BROADCAST_ADDR, (error) => {
      if (error) {
        reject(error);
        return;
      }

      resolve();
    });
  });
}

async function sendGradientFrame({ width, height, rgbType }) {
  const payload = buildGradientFrame(width, height, rgbType);
  const frameCounter = nextFrameCounter;
  const { chunkCount, packets } = splitIntoPackets(width, height, rgbType, frameCounter, payload);

  nextFrameCounter = (nextFrameCounter + 1) & 0xff;

  for (const packet of packets) {
    await sendUdpPacket(packet);
  }

  lastSendResult = {
    width,
    height,
    rgbType,
    frameCounter,
    chunkCount,
    payloadBytes: payload.length,
    packetBytes: packets.map((packet) => packet.length),
  };

  return lastSendResult;
}

function buildSettingsResponse() {
  return {
    ok: true,
    httpPort: HTTP_PORT,
    udpPort: UDP_PORT,
    broadcastAddress: BROADCAST_ADDR,
    defaults: {
      width: DEFAULT_WIDTH,
      height: DEFAULT_HEIGHT,
      rgbType: DEFAULT_RGB_TYPE,
    },
    limits: {
      maxFrameBytes: MAX_FRAME_BYTES,
      maxPayloadBytesPerPacket: MAX_PAYLOAD_SIZE,
      headerSize: HEADER_SIZE,
    },
    rgbTypes: [
      { value: RGB332, label: "RGB332" },
      { value: RGB565, label: "RGB565" },
    ],
    lastSend: {
      ...lastSendResult,
      rgbTypeLabel: rgbTypeName(lastSendResult.rgbType),
    },
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
  const payloadBytes = width * height * bytesPerPixel(rgbType);

  if (payloadBytes > MAX_FRAME_BYTES) {
    res.status(400).json({
      ok: false,
      error: `Frame too large for firmware buffer (${payloadBytes} > ${MAX_FRAME_BYTES} bytes)`,
    });
    return;
  }

  try {
    const result = await sendGradientFrame({ width, height, rgbType });
    res.json({
      ok: true,
      ...result,
      rgbTypeLabel: rgbTypeName(rgbType),
    });
  } catch (error) {
    res.status(500).json({ ok: false, error: error.message });
  }
});

app.listen(HTTP_PORT, () => {
  console.log(`Gradient frame UI on http://localhost:${HTTP_PORT}`);
  console.log(`Broadcasting UDP frames to ${BROADCAST_ADDR}:${UDP_PORT}`);
});
