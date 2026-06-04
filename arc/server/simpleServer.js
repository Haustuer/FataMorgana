const express = require("express");
const dgram = require("dgram");
const path = require("path");

const app = express();
const udp = dgram.createSocket("udp4");

const UDP_PORT = 7777;
const BROADCAST_ADDR = process.env.BROADCAST_ADDR || "255.255.255.255";
const HEADER_SIZE = 8;
const MAX_PACKET_SIZE = 1200;
const MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;

const FRAME_TYPE_CONFIG = 0;
const FRAME_TYPE_IMAGE_START = 1;
const FRAME_TYPE_IMAGE_CONTINUATION = 2;

const RGB332 = 0;
const RGB565 = 1;

const DEFAULT_WIDTH = 24;
const DEFAULT_HEIGHT = 100;
const MAX_FRAME_BYTES = 4800;

let nextFrameCounter = 0;

app.use(express.static(path.join(__dirname, "public")));
app.use(express.json());

udp.bind(() => {
  udp.setBroadcast(true);
});

function normalizeByte(value, fallback) {
  const number = Number(value);

  if (!Number.isFinite(number)) {
    return fallback;
  }

  return Math.max(0, Math.min(255, Math.round(number)));
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

function bytesPerPixel(rgbType) {
  return rgbType === RGB565 ? 2 : 1;
}

function encodeRgb332Pixel(r, g, b) {
  return (r & 0xe0) | ((g & 0xe0) >> 3) | (b >> 6);
}

function encodeRgb565Pixel(r, g, b) {
  return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
}

function buildSolidFrame(width, height, rgbType, color) {
  const pixelCount = width * height;
  const payload = Buffer.alloc(pixelCount * bytesPerPixel(rgbType));

  for (let pixelIndex = 0; pixelIndex < pixelCount; pixelIndex += 1) {
    if (rgbType === RGB565) {
      payload.writeUInt16LE(
        encodeRgb565Pixel(color.r, color.g, color.b),
        pixelIndex * 2,
      );
    } else {
      payload[pixelIndex] = encodeRgb332Pixel(color.r, color.g, color.b);
    }
  }

  return payload;
}

function buildGradientFrame(width, height, rgbType) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = y * width + x;
      const r = Math.round((x / Math.max(1, width - 1)) * 255);
      const g = Math.round((y / Math.max(1, height - 1)) * 255);
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

function buildCheckerFrame(width, height, rgbType) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = y * width + x;
      const isLightSquare = (Math.floor(x / 4) + Math.floor(y / 4)) % 2 === 0;
      const r = isLightSquare ? 255 : 32;
      const g = isLightSquare ? 180 : 16;
      const b = isLightSquare ? 80 : 255;

      if (rgbType === RGB565) {
        payload.writeUInt16LE(encodeRgb565Pixel(r, g, b), pixelIndex * 2);
      } else {
        payload[pixelIndex] = encodeRgb332Pixel(r, g, b);
      }
    }
  }

  return payload;
}

function buildFramePayload(width, height, rgbType, requestBody) {
  const pattern = typeof requestBody.pattern === "string" ? requestBody.pattern : "gradient";

  if (pattern === "solid") {
    return buildSolidFrame(width, height, rgbType, {
      r: normalizeByte(requestBody.r, 255),
      g: normalizeByte(requestBody.g, 64),
      b: normalizeByte(requestBody.b, 16),
    });
  }

  if (pattern === "checker") {
    return buildCheckerFrame(width, height, rgbType);
  }

  return buildGradientFrame(width, height, rgbType);
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

async function broadcastImageFrame(options) {
  const payload = buildFramePayload(options.width, options.height, options.rgbType, options.requestBody);
  const frameCounter = nextFrameCounter;
  const frameInfo = splitIntoPackets(
    options.width,
    options.height,
    options.rgbType,
    frameCounter,
    payload,
  );

  nextFrameCounter = (nextFrameCounter + 1) & 0xff;

  for (const packet of frameInfo.packets) {
    await sendUdpPacket(packet);
  }

  return {
    frameCounter,
    chunkCount: frameInfo.chunkCount,
    payloadBytes: payload.length,
    packetBytes: frameInfo.packets.map((packet) => packet.length),
  };
}

async function handleImageFrameRequest(req, res, patternOverride) {
  const width = normalizeDimension(req.body.width, DEFAULT_WIDTH);
  const height = normalizeDimension(req.body.height, DEFAULT_HEIGHT);
  const rgbType = normalizeRgbType(req.body.rgbType);
  const payloadBytes = width * height * bytesPerPixel(rgbType);

  if (payloadBytes > MAX_FRAME_BYTES) {
    res.status(400).json({
      ok: false,
      error: `Frame too large for current firmware buffer (${payloadBytes} > ${MAX_FRAME_BYTES} bytes)`,
    });
    return;
  }

  try {
    const result = await broadcastImageFrame({
      width,
      height,
      rgbType,
      requestBody: { ...req.body, pattern: patternOverride ?? req.body.pattern },
    });

    res.json({
      ok: true,
      frameCounter: result.frameCounter,
      chunkCount: result.chunkCount,
      payloadBytes: result.payloadBytes,
      packetBytes: result.packetBytes,
      width,
      height,
      rgbType,
    });
  } catch (error) {
    res.status(500).json({ ok: false, error: error.message });
  }
}

app.get("/health", (_req, res) => {
  res.json({
    ok: true,
    headerSize: HEADER_SIZE,
    maxPayloadSize: MAX_PAYLOAD_SIZE,
    defaults: {
      width: DEFAULT_WIDTH,
      height: DEFAULT_HEIGHT,
      rgbType: RGB332,
    },
  });
});

app.post("/frame", async (req, res) => {
  await handleImageFrameRequest(req, res);
});

app.post("/color", async (req, res) => {
  await handleImageFrameRequest(req, res, "solid");
});

app.listen(3000, () => {
  console.log(`Web UI on http://localhost:3000, broadcasting to ${BROADCAST_ADDR}:${UDP_PORT}`);
});
