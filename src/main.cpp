#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "StatusPage.h"

constexpr uint8_t LED_PIN = 12;
constexpr uint16_t LED_COUNT = 256;
constexpr uint16_t MULTICAST_PORT = 7777;
constexpr uint16_t RESPONSE_PORT = 7778;
constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t WS_PORT = 81;
// Multicast group address: 239.255.42.1
const IPAddress MULTICAST_ADDR(239, 255, 42, 1);

constexpr uint8_t FRAME_TYPE_CONFIG = 0;
constexpr uint8_t FRAME_TYPE_IMAGE_START = 1;
constexpr uint8_t FRAME_TYPE_IMAGE_CONTINUATION = 2;

constexpr uint8_t CONFIG_SUBTYPE_DISCOVERY = 0;
constexpr uint8_t CONFIG_SUBTYPE_SET_MAPPING = 1;
constexpr uint8_t CONFIG_SUBTYPE_SET_BRIGHTNESS = 2;

constexpr uint8_t RGB332 = 0;
constexpr uint8_t RGB565 = 1;

constexpr uint8_t MAPPING_ROW = 0;
constexpr uint8_t MAPPING_COLUMN = 1;
constexpr uint8_t MAPPING_RECTANGLE = 2;

constexpr uint8_t SAMPLE_PIXEL = 0;
constexpr uint8_t SAMPLE_INTERPOLATED = 1;

constexpr uint8_t SERPENTINE_NONE = 0;
constexpr uint8_t SERPENTINE_HORIZONTAL = 1;
constexpr uint8_t SERPENTINE_VERTICAL = 2;

constexpr size_t HEADER_SIZE = 8;
constexpr size_t MAX_PACKET_SIZE = 1200;
constexpr size_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
constexpr size_t MAX_FRAME_BYTES = 4800;
constexpr size_t MAX_CHUNKS = (MAX_FRAME_BYTES + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;

struct MappingConfig {
  uint8_t mode = MAPPING_RECTANGLE;
  uint8_t sampleMode = SAMPLE_PIXEL;
  uint16_t rowIndex = 0;
  uint16_t columnIndex = 0;
  uint16_t linePixels = LED_COUNT;
  uint16_t rectX = 0;
  uint16_t rectY = 0;
  uint16_t rectWidth = 10;
  uint16_t rectHeight = 10;
  uint8_t serpentine = SERPENTINE_NONE;  // 0=none, 1=horizontal, 2=vertical
  uint8_t rotation = 0;                   // 0=0°, 1=90°, 2=180°, 3=270°
  bool flipX = false;                     // Horizontal flip (mirror across vertical axis)
  bool flipY = false;                     // Vertical flip (mirror across horizontal axis)
  bool flipZ = false;                     // Diagonal flip (transpose/swap X and Y)
};

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

WiFiUDP udp;
ESP8266WebServer webServer(HTTP_PORT);
WebSocketsServer wsServer(WS_PORT);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiManager wm;

MappingConfig mappingConfig;

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
unsigned long lastStatusBroadcast = 0;
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
  return static_cast<size_t>(width) * static_cast<size_t>(height) * bytesPerPixel(rgbType);
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

const char* mappingModeName(uint8_t mode) {
  if (mode == MAPPING_ROW) {
    return "row";
  }

  if (mode == MAPPING_COLUMN) {
    return "column";
  }

  if (mode == MAPPING_RECTANGLE) {
    return "rectangle";
  }

  return "unknown";
}

const char* sampleModeName(uint8_t mode) {
  if (mode == SAMPLE_PIXEL) {
    return "pixel";
  }

  if (mode == SAMPLE_INTERPOLATED) {
    return "interpolated";
  }

  return "unknown";
}

const char* serpentineModeName(uint8_t mode) {
  if (mode == SERPENTINE_NONE) {
    return "none";
  }

  if (mode == SERPENTINE_HORIZONTAL) {
    return "horizontal";
  }

  if (mode == SERPENTINE_VERTICAL) {
    return "vertical";
  }

  return "unknown";
}

void sanitizeMappingConfig() {
  if (mappingConfig.mode > MAPPING_RECTANGLE) {
    mappingConfig.mode = MAPPING_RECTANGLE;
  }

  if (mappingConfig.sampleMode > SAMPLE_INTERPOLATED) {
    mappingConfig.sampleMode = SAMPLE_PIXEL;
  }

  if (mappingConfig.linePixels < 1) {
    mappingConfig.linePixels = 1;
  }

  if (mappingConfig.linePixels > LED_COUNT) {
    mappingConfig.linePixels = LED_COUNT;
  }

  if (mappingConfig.rectWidth < 1) {
    mappingConfig.rectWidth = 1;
  }

  if (mappingConfig.rectHeight < 1) {
    mappingConfig.rectHeight = 1;
  }

  if (mappingConfig.rotation > 3) {
    mappingConfig.rotation = 0;
  }

  if (mappingConfig.serpentine > SERPENTINE_VERTICAL) {
    mappingConfig.serpentine = SERPENTINE_NONE;
  }
}

void rejectPacket(const char* message) {
  rejectedPackets++;
  Serial.println(message);
}

void writeMappingSummary(char* buffer, size_t bufferSize) {
  if (mappingConfig.mode == MAPPING_ROW) {
    snprintf(buffer,
             bufferSize,
             "row %u, %u leds, %s",
             mappingConfig.rowIndex,
             mappingConfig.linePixels,
             sampleModeName(mappingConfig.sampleMode));
    return;
  }

  if (mappingConfig.mode == MAPPING_COLUMN) {
    snprintf(buffer,
             bufferSize,
             "column %u, %u leds, %s",
             mappingConfig.columnIndex,
             mappingConfig.linePixels,
             sampleModeName(mappingConfig.sampleMode));
    return;
  }

  snprintf(buffer,
           bufferSize,
           "rect %u,%u %ux%u, serpentine %s",
           mappingConfig.rectX,
           mappingConfig.rectY,
           mappingConfig.rectWidth,
           mappingConfig.rectHeight,
           serpentineModeName(mappingConfig.serpentine));
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

RgbColor blackColor() {
  return {0, 0, 0};
}

uint32_t toNeoColor(const RgbColor& color) {
  return strip.Color(color.r, color.g, color.b);
}

void clearStrip(bool showNow) {
  for (uint16_t ledIndex = 0; ledIndex < LED_COUNT; ledIndex++) {
    strip.setPixelColor(ledIndex, 0, 0, 0);
  }

  if (showNow) {
    strip.show();
  }
}

RgbColor decodePixel(size_t pixelIndex) {
  if (activeRgbType == RGB332) {
    const uint8_t packed = frameBuffer[pixelIndex];
    return {
      static_cast<uint8_t>(((packed >> 5) & 0x07) * 255 / 7),
      static_cast<uint8_t>(((packed >> 2) & 0x07) * 255 / 7),
      static_cast<uint8_t>((packed & 0x03) * 255 / 3),
    };
  }

  const size_t offset = pixelIndex * 2;
  const uint16_t packed = static_cast<uint16_t>(frameBuffer[offset]) |
                          (static_cast<uint16_t>(frameBuffer[offset + 1]) << 8);
  return {
    static_cast<uint8_t>(((packed >> 11) & 0x1F) * 255 / 31),
    static_cast<uint8_t>(((packed >> 5) & 0x3F) * 255 / 63),
    static_cast<uint8_t>((packed & 0x1F) * 255 / 31),
  };
}

RgbColor imagePixelAt(int32_t x, int32_t y) {
  if (x < 0 || y < 0) {
    return blackColor();
  }

  if (x >= activeWidth || y >= activeHeight) {
    return blackColor();
  }

  return decodePixel(static_cast<size_t>(y) * activeWidth + x);
}

RgbColor interpolateColors(const RgbColor& left, const RgbColor& right, float factor) {
  return {
    static_cast<uint8_t>(left.r + (right.r - left.r) * factor + 0.5f),
    static_cast<uint8_t>(left.g + (right.g - left.g) * factor + 0.5f),
    static_cast<uint8_t>(left.b + (right.b - left.b) * factor + 0.5f),
  };
}

RgbColor sampleLineColor(bool rowMode, uint16_t fixedIndex, uint16_t ledIndex, uint16_t totalLeds) {
  const uint16_t sourceSpan = rowMode ? activeWidth : activeHeight;
  if (sourceSpan == 0) {
    return blackColor();
  }

  if (mappingConfig.sampleMode == SAMPLE_PIXEL || sourceSpan == 1 || totalLeds == 1) {
    const uint32_t numerator = static_cast<uint32_t>(ledIndex) * static_cast<uint32_t>(sourceSpan - 1);
    const uint16_t sourceIndex = totalLeds <= 1
      ? 0
      : static_cast<uint16_t>((numerator + (totalLeds - 1) / 2) / (totalLeds - 1));
    return rowMode
      ? imagePixelAt(sourceIndex, fixedIndex)
      : imagePixelAt(fixedIndex, sourceIndex);
  }

  const float position = (static_cast<float>(ledIndex) * static_cast<float>(sourceSpan - 1)) /
                         static_cast<float>(totalLeds - 1);
  const uint16_t lowerIndex = static_cast<uint16_t>(position);
  uint16_t upperIndex = lowerIndex + 1;
  if (upperIndex >= sourceSpan) {
    upperIndex = sourceSpan - 1;
  }

  const float factor = position - lowerIndex;
  const RgbColor lowerColor = rowMode
    ? imagePixelAt(lowerIndex, fixedIndex)
    : imagePixelAt(fixedIndex, lowerIndex);
  const RgbColor upperColor = rowMode
    ? imagePixelAt(upperIndex, fixedIndex)
    : imagePixelAt(fixedIndex, upperIndex);

  return interpolateColors(lowerColor, upperColor, factor);
}

void renderFrame() {
  clearStrip(false);

  if (activeWidth == 0 || activeHeight == 0) {
    strip.show();
    return;
  }

  if (mappingConfig.mode == MAPPING_ROW) {
    if (mappingConfig.rowIndex < activeHeight) {
      for (uint16_t ledIndex = 0; ledIndex < mappingConfig.linePixels; ledIndex++) {
        strip.setPixelColor(
          ledIndex,
          toNeoColor(sampleLineColor(true, mappingConfig.rowIndex, ledIndex, mappingConfig.linePixels))
        );
      }
    }
  } else if (mappingConfig.mode == MAPPING_COLUMN) {
    if (mappingConfig.columnIndex < activeWidth) {
      for (uint16_t ledIndex = 0; ledIndex < mappingConfig.linePixels; ledIndex++) {
        strip.setPixelColor(
          ledIndex,
          toNeoColor(sampleLineColor(false, mappingConfig.columnIndex, ledIndex, mappingConfig.linePixels))
        );
      }
    }
  } else {
    const size_t rectanglePixels = static_cast<size_t>(mappingConfig.rectWidth) * mappingConfig.rectHeight;
    const size_t ledCount = rectanglePixels < LED_COUNT ? rectanglePixels : LED_COUNT;

    for (size_t ledIndex = 0; ledIndex < ledCount; ledIndex++) {
      uint16_t row = static_cast<uint16_t>(ledIndex / mappingConfig.rectWidth);
      if (row >= mappingConfig.rectHeight) {
        break;
      }

      uint16_t column = static_cast<uint16_t>(ledIndex % mappingConfig.rectWidth);

      // Apply serpentine patterns
      if (mappingConfig.serpentine == SERPENTINE_HORIZONTAL && (row % 2 == 1)) {
        // Horizontal serpentine: reverse columns on odd rows (zigzag left-right)
        column = mappingConfig.rectWidth - 1 - column;
      } else if (mappingConfig.serpentine == SERPENTINE_VERTICAL && (column % 2 == 1)) {
        // Vertical serpentine: reverse rows on odd columns (zigzag up-down)
        row = mappingConfig.rectHeight - 1 - row;
      }

      const uint16_t sourceX = mappingConfig.rectX + column;
      const uint16_t sourceY = mappingConfig.rectY + row;
      strip.setPixelColor(static_cast<uint16_t>(ledIndex), toNeoColor(imagePixelAt(sourceX, sourceY)));
    }
  }

  strip.show();
}

String buildStatusJson() {
  JsonDocument doc;
  char mapping[64];
  char lastFrame[32];
  char imageSize[20];
  char chunks[32];
  char packets[64];
  char lastRender[40];

  writeMappingSummary(mapping, sizeof(mapping));
  writeLastFrameString(lastFrame, sizeof(lastFrame));
  writeImageSizeString(imageSize, sizeof(imageSize));
  writeChunkString(chunks, sizeof(chunks));
  writePacketString(packets, sizeof(packets));
  writeLastRenderString(lastRender, sizeof(lastRender));

  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["wifiStatus"] = wifiStatusName(WiFi.status());
  doc["multicastGroup"] = MULTICAST_ADDR.toString();
  doc["multicastPort"] = MULTICAST_PORT;
  doc["responsePort"] = RESPONSE_PORT;
  doc["ledCount"] = LED_COUNT;
  doc["mapping"] = mapping;
  doc["mappingMode"] = mappingModeName(mappingConfig.mode);
  doc["sampleMode"] = sampleModeName(mappingConfig.sampleMode);
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

String buildConfigJson() {
  JsonDocument doc;

  doc["mode"] = mappingConfig.mode;
  doc["sampleMode"] = mappingConfig.sampleMode;
  doc["rowIndex"] = mappingConfig.rowIndex;
  doc["columnIndex"] = mappingConfig.columnIndex;
  doc["linePixels"] = mappingConfig.linePixels;
  doc["rectX"] = mappingConfig.rectX;
  doc["rectY"] = mappingConfig.rectY;
  doc["rectWidth"] = mappingConfig.rectWidth;
  doc["rectHeight"] = mappingConfig.rectHeight;
  doc["serpentine"] = mappingConfig.serpentine;
  doc["rotation"] = mappingConfig.rotation;
  doc["flipX"] = mappingConfig.flipX;
  doc["flipY"] = mappingConfig.flipY;
  doc["flipZ"] = mappingConfig.flipZ;
  doc["ledCount"] = LED_COUNT;

  String json;
  serializeJson(doc, json);
  return json;
}

// Forward declaration for WebSocket broadcast
void broadcastStatus();

void sendDiscoveryResponse(IPAddress serverIP, uint16_t serverPort) {
  uint8_t response[64];
  memset(response, 0, sizeof(response));

  // Magic bytes "FATA" (0x46415441)
  response[0] = 0x46; // 'F'
  response[1] = 0x41; // 'A'
  response[2] = 0x54; // 'T'
  response[3] = 0x41; // 'A'

  // Protocol version and type
  response[4] = 1;    // Version
  response[5] = 0x01; // Discovery response type
  response[6] = 0;    // Reserved
  response[7] = 0;    // Reserved

  // Device IP (network byte order - big-endian)
  IPAddress localIP = WiFi.localIP();
  response[8] = localIP[0];
  response[9] = localIP[1];
  response[10] = localIP[2];
  response[11] = localIP[3];

  // MAC address (6 bytes)
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(response + 12, mac, 6);

  // Chip ID (little-endian)
  uint32_t chipId = ESP.getChipId();
  response[18] = chipId & 0xFF;
  response[19] = (chipId >> 8) & 0xFF;
  response[20] = (chipId >> 16) & 0xFF;
  response[21] = (chipId >> 24) & 0xFF;

  // Uptime (little-endian)
  uint32_t uptime = millis();
  response[22] = uptime & 0xFF;
  response[23] = (uptime >> 8) & 0xFF;
  response[24] = (uptime >> 16) & 0xFF;
  response[25] = (uptime >> 24) & 0xFF;

  // LED hardware info
  response[26] = LED_COUNT & 0xFF;
  response[27] = (LED_COUNT >> 8) & 0xFF;
  response[28] = LED_PIN;
  response[29] = strip.getBrightness();
  response[30] = 1; // Firmware major version
  response[31] = 0; // Firmware minor version

  // Mapping configuration
  response[32] = mappingConfig.mode;
  response[33] = mappingConfig.sampleMode;

  // Row/Column index (same field, little-endian)
  response[34] = mappingConfig.rowIndex & 0xFF;
  response[35] = (mappingConfig.rowIndex >> 8) & 0xFF;

  // Line pixels (little-endian)
  response[36] = mappingConfig.linePixels & 0xFF;
  response[37] = (mappingConfig.linePixels >> 8) & 0xFF;

  // Rectangle configuration (little-endian)
  response[38] = mappingConfig.rectX & 0xFF;
  response[39] = (mappingConfig.rectX >> 8) & 0xFF;
  response[40] = mappingConfig.rectY & 0xFF;
  response[41] = (mappingConfig.rectY >> 8) & 0xFF;
  response[42] = mappingConfig.rectWidth & 0xFF;
  response[43] = (mappingConfig.rectWidth >> 8) & 0xFF;
  response[44] = mappingConfig.rectHeight & 0xFF;
  response[45] = (mappingConfig.rectHeight >> 8) & 0xFF;
  response[46] = mappingConfig.serpentine;  // 0=none, 1=horizontal, 2=vertical

  // Transform byte: rotation (bits 0-1), flipX (bit 2), flipY (bit 3), flipZ (bit 4)
  response[47] = (mappingConfig.rotation & 0x03) |
                 (mappingConfig.flipX ? 0x04 : 0) |
                 (mappingConfig.flipY ? 0x08 : 0) |
                 (mappingConfig.flipZ ? 0x10 : 0);

  // Statistics (little-endian)
  response[48] = acceptedPackets & 0xFF;
  response[49] = (acceptedPackets >> 8) & 0xFF;
  response[50] = (acceptedPackets >> 16) & 0xFF;
  response[51] = (acceptedPackets >> 24) & 0xFF;

  response[52] = rejectedPackets & 0xFF;
  response[53] = (rejectedPackets >> 8) & 0xFF;
  response[54] = (rejectedPackets >> 16) & 0xFF;
  response[55] = (rejectedPackets >> 24) & 0xFF;

  response[56] = renderedFrames & 0xFF;
  response[57] = (renderedFrames >> 8) & 0xFF;
  response[58] = (renderedFrames >> 16) & 0xFF;
  response[59] = (renderedFrames >> 24) & 0xFF;

  // Last frame info (little-endian)
  response[60] = lastSeenWidth & 0xFF;
  response[61] = (lastSeenWidth >> 8) & 0xFF;
  response[62] = lastSeenHeight & 0xFF;
  response[63] = (lastSeenHeight >> 8) & 0xFF;

  // Send binary UDP packet
  udp.beginPacket(serverIP, serverPort);
  udp.write(response, sizeof(response));
  udp.endPacket();

  Serial.printf("Sent binary discovery response to %s:%u (64 bytes)\n",
                serverIP.toString().c_str(),
                serverPort);
}

void handleWebRoot() {
  webServer.send_P(200, PSTR("text/html"), kStatusPageHtml);
}

void handleWebStatus() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", buildStatusJson());
}

void handleWebGetConfig() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", buildConfigJson());
}

void handleWebSetConfig() {
  if (!webServer.hasArg("plain")) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"missing request body\"}");
    return;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  if (error) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid json\"}");
    return;
  }

  if (doc["mode"].is<uint8_t>()) {
    mappingConfig.mode = doc["mode"];
  }

  if (doc["sampleMode"].is<uint8_t>()) {
    mappingConfig.sampleMode = doc["sampleMode"];
  }

  if (doc["rowIndex"].is<uint16_t>()) {
    mappingConfig.rowIndex = doc["rowIndex"];
  }

  if (doc["columnIndex"].is<uint16_t>()) {
    mappingConfig.columnIndex = doc["columnIndex"];
  }

  if (doc["linePixels"].is<uint16_t>()) {
    mappingConfig.linePixels = doc["linePixels"];
  }

  if (doc["rectX"].is<uint16_t>()) {
    mappingConfig.rectX = doc["rectX"];
  }

  if (doc["rectY"].is<uint16_t>()) {
    mappingConfig.rectY = doc["rectY"];
  }

  if (doc["rectWidth"].is<uint16_t>()) {
    mappingConfig.rectWidth = doc["rectWidth"];
  }

  if (doc["rectHeight"].is<uint16_t>()) {
    mappingConfig.rectHeight = doc["rectHeight"];
  }

  if (doc["serpentine"].is<uint8_t>()) {
    mappingConfig.serpentine = doc["serpentine"];
  }

  if (doc["rotation"].is<uint8_t>()) {
    mappingConfig.rotation = doc["rotation"];
  }

  if (doc["flipX"].is<bool>()) {
    mappingConfig.flipX = doc["flipX"];
  }

  if (doc["flipY"].is<bool>()) {
    mappingConfig.flipY = doc["flipY"];
  }

  if (doc["flipZ"].is<bool>()) {
    mappingConfig.flipZ = doc["flipZ"];
  }

  sanitizeMappingConfig();

  if (!frameInProgress && activeWidth > 0 && activeHeight > 0) {
    renderFrame();
    lastRenderMillis = millis();
    renderedFrames++;

    // Broadcast updated status immediately via WebSocket
    broadcastStatus();
  }

  webServer.send(200, "application/json", buildConfigJson());
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("WebSocket [%u] disconnected\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = wsServer.remoteIP(num);
        Serial.printf("WebSocket [%u] connected from %s\n", num, ip.toString().c_str());
        // Send initial status on connect
        String statusJson = buildStatusJson();
        wsServer.sendTXT(num, statusJson);
      }
      break;
    case WStype_TEXT:
      Serial.printf("WebSocket [%u] received text: %s\n", num, payload);
      break;
    case WStype_BIN:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
    default:
      // Ignore other WebSocket event types
      break;
  }
}

void broadcastStatus() {
  if (wsServer.connectedClients() > 0) {
    String statusJson = buildStatusJson();
    wsServer.broadcastTXT(statusJson);
  }
}

void startWebServer() {
  webServer.on("/", HTTP_GET, handleWebRoot);
  webServer.on("/status.json", HTTP_GET, handleWebStatus);
  webServer.on("/config.json", HTTP_GET, handleWebGetConfig);
  webServer.on("/config", HTTP_POST, handleWebSetConfig);
  webServer.begin();

  wsServer.begin();
  wsServer.onEvent(webSocketEvent);

  const String localIp = WiFi.localIP().toString();
  Serial.printf("HTTP status page on %s:%u\n", localIp.c_str(), HTTP_PORT);
  Serial.printf("WebSocket server on ws://%s:%u/ws\n", localIp.c_str(), WS_PORT);
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
  lastPacketMillis = millis();

  // Handle config frames
  if (frameType == FRAME_TYPE_CONFIG) {
    const uint8_t subType = data[3]; // TypeData field contains subtype
    lastSeenRgbType = subType; // Store subtype for logging
    lastPayloadBytes = payloadLength;

    if (subType == CONFIG_SUBTYPE_DISCOVERY) {
      // Discovery request: payload contains server IP (4 bytes) + port (2 bytes)
      if (payloadLength >= 6) {
        // Parse server IP (big-endian/network byte order)
        IPAddress serverIP(payload[0], payload[1], payload[2], payload[3]);

        // Parse response port (big-endian/network byte order)
        uint16_t serverPort = (static_cast<uint16_t>(payload[4]) << 8) | payload[5];

        Serial.printf("Discovery request from %s:%u\n",
                      serverIP.toString().c_str(),
                      serverPort);

        sendDiscoveryResponse(serverIP, serverPort);
        acceptedPackets++;
      } else {
        Serial.println("Invalid discovery request: payload too short");
        rejectedPackets++;
      }
    } else {
      Serial.printf("Unknown config subtype: %u\n", subType);
      rejectedPackets++;
    }
    return;
  }

  // Handle image frames - store width/height/rgbType for tracking
  lastSeenRgbType = rgbType;
  lastPayloadBytes = payloadLength;
  lastSeenWidth = width;
  lastSeenHeight = height;

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

  sanitizeMappingConfig();

  strip.begin();
  strip.show();
  strip.setBrightness(80);
  clearStrip(true);

  wm.setConfigPortalBlocking(false);
  wm.autoConnect("LED-Setup");

  // Join multicast group for receiving frames
  if (udp.beginMulticast(WiFi.localIP(), MULTICAST_ADDR, MULTICAST_PORT)) {
    Serial.printf("Joined multicast group %s:%u\n",
                  MULTICAST_ADDR.toString().c_str(),
                  MULTICAST_PORT);
  } else {
    Serial.println("Failed to join multicast group!");
  }

  startWebServer();
  Serial.printf("Device IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Device MAC: %s\n", WiFi.macAddress().c_str());
}

void loop() {
  wm.process();
  webServer.handleClient();
  wsServer.loop();

  // Broadcast status updates every 200ms if there are connected clients
  unsigned long now = millis();
  if (wsServer.connectedClients() > 0 && (now - lastStatusBroadcast >= 200)) {
    broadcastStatus();
    lastStatusBroadcast = now;
  }

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
