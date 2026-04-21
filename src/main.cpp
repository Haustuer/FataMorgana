#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "StatusPage.h"

constexpr uint8_t LED_PIN = 12;
constexpr uint16_t LED_COUNT = 100;
constexpr uint16_t UDP_PORT = 7777;
constexpr uint16_t HTTP_PORT = 80;

constexpr uint8_t FRAME_TYPE_CONFIG = 0;
constexpr uint8_t FRAME_TYPE_IMAGE_START = 1;
constexpr uint8_t FRAME_TYPE_IMAGE_CONTINUATION = 2;

constexpr uint8_t RGB332 = 0;
constexpr uint8_t RGB565 = 1;

constexpr size_t HEADER_SIZE = 8;
constexpr size_t MAX_PACKET_SIZE = 1200;
constexpr size_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
constexpr size_t MAX_FRAME_BYTES = 4800;
constexpr size_t MAX_CHUNKS = (MAX_FRAME_BYTES + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;

constexpr uint16_t REGION_X = 0;
constexpr uint16_t REGION_Y = 0;
constexpr uint16_t REGION_WIDTH = 10;
constexpr uint16_t REGION_HEIGHT = 10;

static_assert(REGION_WIDTH * REGION_HEIGHT <= LED_COUNT, "Selected region must fit on the LED strip.");

WiFiUDP udp;
ESP8266WebServer webServer(HTTP_PORT);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiManager wm;

uint8_t packetBuffer[MAX_PACKET_SIZE];
uint8_t frameBuffer[MAX_FRAME_BYTES];
bool chunkReceived[MAX_CHUNKS];

bool frameInProgress = false;
uint8_t activeFrameCounter = 0;
uint8_t activeRgbType = RGB332;
uint16_t activeWidth = 0;
uint16_t activeHeight = 0;
size_t activeFrameBytes = 0;
uint8_t expectedChunkCount = 0;
uint8_t receivedChunkCount = 0;
uint8_t lastFrameType = FRAME_TYPE_CONFIG;
uint8_t lastSeenFrameCounter = 0;
uint8_t lastChunkIndex = 0;
uint8_t lastSeenRgbType = RGB332;
size_t lastPayloadBytes = 0;
uint16_t lastSeenWidth = 0;
uint16_t lastSeenHeight = 0;
unsigned long lastPacketMillis = 0;
unsigned long lastRenderMillis = 0;
uint32_t acceptedPackets = 0;
uint32_t rejectedPackets = 0;
uint32_t renderedFrames = 0;

size_t bytesPerPixel(uint8_t rgbType) {
  if (rgbType == RGB332) {
    return 1;
  }

  if (rgbType == RGB565) {
    return 2;
  }

  return 0;
}

size_t totalFrameBytes(uint16_t width, uint16_t height, uint8_t rgbType) {
  const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
  return pixelCount * bytesPerPixel(rgbType);
}

uint8_t deriveChunkCount(size_t frameBytes) {
  if (frameBytes == 0) {
    return 0;
  }

  return static_cast<uint8_t>((frameBytes + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE);
}

const char* rgbTypeName(uint8_t rgbType) {
  if (rgbType == RGB332) {
    return "RGB332";
  }

  if (rgbType == RGB565) {
    return "RGB565";
  }

  return "unknown";
}

const char* frameTypeName(uint8_t frameType) {
  if (frameType == FRAME_TYPE_CONFIG) {
    return "config";
  }

  if (frameType == FRAME_TYPE_IMAGE_START) {
    return "image-start";
  }

  if (frameType == FRAME_TYPE_IMAGE_CONTINUATION) {
    return "image-continuation";
  }

  return "unknown";
}

const char* wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:
      return "connected";
    case WL_NO_SSID_AVAIL:
      return "no-ssid";
    case WL_CONNECT_FAILED:
      return "connect-failed";
    case WL_CONNECTION_LOST:
      return "connection-lost";
    case WL_DISCONNECTED:
      return "disconnected";
    case WL_IDLE_STATUS:
      return "idle";
    default:
      return "unknown";
  }
}

void rejectPacket(const char* message) {
  rejectedPackets++;
  Serial.println(message);
}

void writeRegionString(char* buffer, size_t bufferSize) {
  snprintf(buffer,
           bufferSize,
           "%u,%u %ux%u",
           REGION_X,
           REGION_Y,
           REGION_WIDTH,
           REGION_HEIGHT);
}

void writeLastFrameString(char* buffer, size_t bufferSize) {
  snprintf(buffer,
           bufferSize,
           "%s #%u",
           frameTypeName(lastFrameType),
           lastSeenFrameCounter);
}

void writeImageSizeString(char* buffer, size_t bufferSize) {
  snprintf(buffer,
           bufferSize,
           "%ux%u",
           lastSeenWidth,
           lastSeenHeight);
}

void writeChunkString(char* buffer, size_t bufferSize) {
  snprintf(buffer,
           bufferSize,
           "%u/%u current chunk %u",
           receivedChunkCount,
           expectedChunkCount,
           lastChunkIndex);
}

void writePacketString(char* buffer, size_t bufferSize) {
  snprintf(buffer,
           bufferSize,
           "%lu accepted, %lu rejected, last payload %u bytes",
           static_cast<unsigned long>(acceptedPackets),
           static_cast<unsigned long>(rejectedPackets),
           static_cast<unsigned int>(lastPayloadBytes));
}

void writeLastRenderString(char* buffer, size_t bufferSize) {
  snprintf(buffer,
           bufferSize,
           "%lu ms, last packet %lu ms",
           lastRenderMillis,
           lastPacketMillis);
}

String buildStatusJson() {
  StaticJsonDocument<512> doc;
  char region[32];
  char lastFrame[32];
  char imageSize[20];
  char chunks[32];
  char packets[64];
  char lastRender[40];

  writeRegionString(region, sizeof(region));
  writeLastFrameString(lastFrame, sizeof(lastFrame));
  writeImageSizeString(imageSize, sizeof(imageSize));
  writeChunkString(chunks, sizeof(chunks));
  writePacketString(packets, sizeof(packets));
  writeLastRenderString(lastRender, sizeof(lastRender));

  doc["ip"] = WiFi.localIP().toString();
  doc["wifiStatus"] = wifiStatusName(WiFi.status());
  doc["udpPort"] = UDP_PORT;
  doc["ledCount"] = LED_COUNT;
  doc["region"] = region;
  doc["lastFrame"] = lastFrame;
  doc["imageSize"] = imageSize;
  doc["rgbType"] = rgbTypeName(lastSeenRgbType);
  doc["chunks"] = chunks;
  doc["packets"] = packets;
  doc["renderedFrames"] = renderedFrames;
  doc["lastRender"] = lastRender;
  doc["frameInProgress"] = frameInProgress;

  String json;
  serializeJson(doc, json);
  return json;
}

void handleWebRoot() {
  webServer.send_P(200, PSTR("text/html"), kStatusPageHtml);
}

void handleWebStatus() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", buildStatusJson());
}

void startWebServer() {
  webServer.on("/", HTTP_GET, handleWebRoot);
  webServer.on("/status.json", HTTP_GET, handleWebStatus);
  webServer.begin();
  const String localIp = WiFi.localIP().toString();
  Serial.printf("HTTP status page on %s:%u\n", localIp.c_str(), HTTP_PORT);
}

void clearStrip() {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
}

uint32_t decodePixel(size_t pixelIndex) {
  if (activeRgbType == RGB332) {
    const uint8_t packed = frameBuffer[pixelIndex];
    const uint8_t r = static_cast<uint8_t>(((packed >> 5) & 0x07) * 255 / 7);
    const uint8_t g = static_cast<uint8_t>(((packed >> 2) & 0x07) * 255 / 7);
    const uint8_t b = static_cast<uint8_t>((packed & 0x03) * 255 / 3);
    return strip.Color(r, g, b);
  }

  const size_t offset = pixelIndex * 2;
  const uint16_t packed = static_cast<uint16_t>(frameBuffer[offset]) |
                          (static_cast<uint16_t>(frameBuffer[offset + 1]) << 8);
  const uint8_t r = static_cast<uint8_t>(((packed >> 11) & 0x1F) * 255 / 31);
  const uint8_t g = static_cast<uint8_t>(((packed >> 5) & 0x3F) * 255 / 63);
  const uint8_t b = static_cast<uint8_t>((packed & 0x1F) * 255 / 31);
  return strip.Color(r, g, b);
}

void renderFrame() {
  uint16_t ledIndex = 0;

  for (uint16_t y = 0; y < REGION_HEIGHT && ledIndex < LED_COUNT; y++) {
    for (uint16_t x = 0; x < REGION_WIDTH && ledIndex < LED_COUNT; x++) {
      const uint16_t sourceX = REGION_X + x;
      const uint16_t sourceY = REGION_Y + y;

      if (sourceX < activeWidth && sourceY < activeHeight) {
        const size_t pixelIndex = static_cast<size_t>(sourceY) * activeWidth + sourceX;
        strip.setPixelColor(ledIndex, decodePixel(pixelIndex));
      } else {
        strip.setPixelColor(ledIndex, 0, 0, 0);
      }

      ledIndex++;
    }
  }

  while (ledIndex < LED_COUNT) {
    strip.setPixelColor(ledIndex, 0, 0, 0);
    ledIndex++;
  }

  strip.show();
}

bool beginFrame(uint8_t frameCounter, uint8_t rgbType, uint16_t width, uint16_t height) {
  const size_t frameBytes = totalFrameBytes(width, height, rgbType);
  const uint8_t chunkCount = deriveChunkCount(frameBytes);

  if (frameBytes == 0 || frameBytes > MAX_FRAME_BYTES || chunkCount == 0 || chunkCount > MAX_CHUNKS) {
    Serial.println("Rejected frame: unsupported size or RGB type");
    return false;
  }

  activeFrameCounter = frameCounter;
  activeRgbType = rgbType;
  activeWidth = width;
  activeHeight = height;
  activeFrameBytes = frameBytes;
  expectedChunkCount = chunkCount;
  receivedChunkCount = 0;
  frameInProgress = true;

  memset(frameBuffer, 0, sizeof(frameBuffer));
  memset(chunkReceived, 0, sizeof(chunkReceived));

  return true;
}

bool validateActiveFrame(uint8_t frameCounter, uint8_t rgbType, uint16_t width, uint16_t height) {
  return frameInProgress &&
         frameCounter == activeFrameCounter &&
         rgbType == activeRgbType &&
         width == activeWidth &&
         height == activeHeight;
}

bool storeChunk(uint8_t chunkIndex, const uint8_t* payload, size_t payloadLength) {
  if (!frameInProgress || chunkIndex >= expectedChunkCount) {
    return false;
  }

  const size_t offset = static_cast<size_t>(chunkIndex) * MAX_PAYLOAD_SIZE;
  const size_t remaining = activeFrameBytes - offset;
  const size_t expectedLength = remaining < MAX_PAYLOAD_SIZE ? remaining : MAX_PAYLOAD_SIZE;

  if (payloadLength != expectedLength) {
    Serial.println("Rejected frame chunk: unexpected payload length");
    return false;
  }

  memcpy(frameBuffer + offset, payload, payloadLength);

  if (!chunkReceived[chunkIndex]) {
    chunkReceived[chunkIndex] = true;
    receivedChunkCount++;
  }

  return true;
}

void handleFramePacket(const uint8_t* data, size_t length) {
  if (length < HEADER_SIZE) {
    return;
  }

  const uint8_t frameType = data[0];
  const uint8_t frameCounter = data[1];
  const uint8_t chunkIndex = data[2];
  const uint8_t rgbType = data[3];
  const uint16_t width = static_cast<uint16_t>(data[4]) |
                         (static_cast<uint16_t>(data[5]) << 8);
  const uint16_t height = static_cast<uint16_t>(data[6]) |
                          (static_cast<uint16_t>(data[7]) << 8);
  const uint8_t* payload = data + HEADER_SIZE;
  const size_t payloadLength = length - HEADER_SIZE;

  lastFrameType = frameType;
  lastSeenFrameCounter = frameCounter;
  lastChunkIndex = chunkIndex;
  lastSeenRgbType = rgbType;
  lastPayloadBytes = payloadLength;
  lastSeenWidth = width;
  lastSeenHeight = height;
  lastPacketMillis = millis();

  if (frameType == FRAME_TYPE_CONFIG) {
    Serial.println("Config frames are not implemented yet");
    return;
  }

  if (frameType == FRAME_TYPE_IMAGE_START) {
    if (chunkIndex != 0 || !beginFrame(frameCounter, rgbType, width, height)) {
      rejectPacket("Rejected frame start packet");
      return;
    }
  } else if (frameType == FRAME_TYPE_IMAGE_CONTINUATION) {
    if (!validateActiveFrame(frameCounter, rgbType, width, height)) {
      rejectPacket("Rejected continuation frame: active frame mismatch");
      return;
    }
  } else {
    rejectPacket("Rejected frame: unknown frame type");
    return;
  }

  if (!storeChunk(chunkIndex, payload, payloadLength)) {
    rejectPacket("Rejected frame chunk");
    return;
  }

  acceptedPackets++;

  if (receivedChunkCount == expectedChunkCount) {
    Serial.printf("Rendering frame %u (%ux%u, rgbType=%u)\n",
                  activeFrameCounter,
                  activeWidth,
                  activeHeight,
                  activeRgbType);
    renderFrame();
    renderedFrames++;
    lastRenderMillis = millis();
    frameInProgress = false;
  }
}

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.show();
  strip.setBrightness(80);
  clearStrip();

  wm.setConfigPortalBlocking(false);
  wm.autoConnect("LED-Setup");

  udp.begin(UDP_PORT);
  startWebServer();
  Serial.printf("UDP listening on %u\n", UDP_PORT);
}

void loop() {
  wm.process();
  webServer.handleClient();

  const int packetSize = udp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  if (packetSize > static_cast<int>(sizeof(packetBuffer))) {
    Serial.println("Rejected packet: exceeds packet buffer");
    return;
  }

  const int length = udp.read(packetBuffer, sizeof(packetBuffer));
  if (length > 0) {
    handleFramePacket(packetBuffer, static_cast<size_t>(length));
  }
}
