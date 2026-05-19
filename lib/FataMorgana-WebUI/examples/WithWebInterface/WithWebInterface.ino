/**
 * @file WithWebInterface.ino
 * @brief FataMorgana with web configuration interface
 *
 * This example demonstrates using the FataMorgana-WebUI library to provide
 * a web-based configuration interface for your LED display device.
 *
 * Features:
 * - Web interface on port 80 for configuration
 * - WebSocket real-time status updates on port 81
 * - Auto-save on configuration changes
 * - Immediate re-render when mapping changes
 * - Statistics display every 10 seconds
 *
 * Hardware:
 * - ESP8266 or ESP32
 * - WS2812B LED strip/matrix connected to GPIO 12
 * - 256 LEDs (adjustable)
 *
 * Setup:
 * 1. Update WiFi credentials below
 * 2. Upload sketch to ESP
 * 3. Open Serial Monitor (115200 baud) to see IP address
 * 4. Access web interface at http://<device-ip>
 *
 * @author FataMorgana
 * @date 2026
 */

#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

constexpr uint8_t LED_PIN = 12;
constexpr uint16_t LED_COUNT = 256;  // 16x16 matrix
constexpr uint8_t LED_TYPE = NEO_GRB + NEO_KHZ800;  // WS2812B

// Web server ports
constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t WS_PORT = 81;

// ============================================================================
// GLOBALS
// ============================================================================

FataMorganaClient client(LED_COUNT, LED_PIN, LED_TYPE);
FataMorganaWebUI webUI(client);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println(F("FataMorgana with WebUI"));
  Serial.println(F("======================"));

  // Connect to WiFi
  Serial.printf("Connecting to WiFi '%s'", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println();
  Serial.print(F("Connected! IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("MAC: "));
  Serial.println(WiFi.macAddress());

  // Initialize FataMorgana client
  if (!client.begin()) {
    Serial.println(F("ERROR: Failed to initialize FataMorgana!"));
    Serial.println(F("Check multicast support on your network."));
    while (1) {
      delay(1000);
    }
  }

  Serial.println(F("FataMorgana client initialized"));

  // Configure default mapping (16x16 rectangle at origin)
  client.setRectangle(0, 0, 16, 16);
  client.setSerpentine(SERPENTINE_HORIZONTAL);
  client.setRotation(0);
  client.setBrightness(80);

  // Start web interface
  if (!webUI.begin(HTTP_PORT, WS_PORT)) {
    Serial.println(F("ERROR: Failed to start web server!"));
    Serial.printf("Ports %u (HTTP) and %u (WebSocket) may be in use.\n", HTTP_PORT, WS_PORT);
  } else {
    Serial.println(F("Web interface started"));
    Serial.printf("HTTP server: http://%s:%u\n", WiFi.localIP().toString().c_str(), HTTP_PORT);
    Serial.printf("WebSocket:   ws://%s:%u\n", WiFi.localIP().toString().c_str(), WS_PORT);
  }

  // Print initial configuration
  printConfiguration();

  Serial.println();
  Serial.println(F("Ready! Waiting for frames..."));
  Serial.println(F("Configure at: http://") + WiFi.localIP().toString());
  Serial.println();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Process FataMorgana protocol
  client.loop();

  // Process web interface
  webUI.loop();

  // Print statistics every 10 seconds
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {
    lastStats = millis();
    printStatistics();
  }

  // Keep WiFi connection alive
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi disconnected! Reconnecting..."));
    WiFi.reconnect();
  }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void printConfiguration() {
  const FataMorganaMapping& mapping = client.getMapping();

  Serial.println(F("\n--- Current Configuration ---"));
  Serial.print(F("Mode: "));
  Serial.println(fatamorgana_mappingModeName(mapping.mode));

  if (mapping.mode == MAPPING_ROW) {
    Serial.printf("  Row Index: %u\n", mapping.rowIndex);
    Serial.printf("  Pixels: %u\n", mapping.linePixels);
    Serial.print(F("  Sample Mode: "));
    Serial.println(fatamorgana_sampleModeName(mapping.sampleMode));
  } else if (mapping.mode == MAPPING_COLUMN) {
    Serial.printf("  Column Index: %u\n", mapping.columnIndex);
    Serial.printf("  Pixels: %u\n", mapping.linePixels);
    Serial.print(F("  Sample Mode: "));
    Serial.println(fatamorgana_sampleModeName(mapping.sampleMode));
  } else {
    Serial.printf("  Rectangle: (%u,%u) %ux%u\n",
                  mapping.rectX, mapping.rectY,
                  mapping.rectWidth, mapping.rectHeight);
    Serial.print(F("  Serpentine: "));
    Serial.println(fatamorgana_serpentineModeName(mapping.serpentine));
  }

  Serial.printf("Rotation: %u° (%s)\n",
                mapping.rotation * 90,
                mapping.rotation == 0 ? "0°" :
                mapping.rotation == 1 ? "90° CW" :
                mapping.rotation == 2 ? "180°" : "270° CW");
  Serial.printf("Flips: X=%d Y=%d Z=%d\n", mapping.flipX, mapping.flipY, mapping.flipZ);
  Serial.println();
}

void printStatistics() {
  Serial.println(F("--- Statistics ---"));
  Serial.printf("Rendered Frames: %u\n", client.getRenderedFrames());
  Serial.printf("Accepted Packets: %u\n", client.getAcceptedPackets());
  Serial.printf("Rejected Packets: %u\n", client.getRejectedPackets());

  uint32_t total = client.getAcceptedPackets() + client.getRejectedPackets();
  if (total > 0) {
    float loss = 100.0f * client.getRejectedPackets() / total;
    Serial.printf("Packet Loss: %.2f%%\n", loss);
  }

  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println();
}
