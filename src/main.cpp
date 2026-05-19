/**
 * @file main.cpp
 * @brief FataMorgana LED Display - Example using library
 *
 * This example demonstrates using the FataMorgana library with
 * optional WebUI component for web-based configuration.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// FataMorgana Core Library
#include <FataMorgana.h>

// FataMorgana WebUI (Optional - comment out if not needed)
#include <FataMorganaWebUI.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

constexpr uint8_t LED_PIN = 12;       // GPIO pin for LED data
constexpr uint16_t LED_COUNT = 100;   // Total number of LEDs
constexpr uint8_t LED_TYPE = NEO_GRB + NEO_KHZ800;  // NeoPixel type

// ============================================================================
// GLOBALS
// ============================================================================

WiFiManager wifiManager;
FataMorganaClient client(LED_COUNT, LED_PIN, LED_TYPE);
FataMorganaWebUI webUI(client);  // Optional - comment out if not using WebUI

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("====================================="));
  Serial.println(F("FataMorgana LED Display"));
  Serial.println(F("====================================="));

  // Initialize WiFi with WiFiManager (creates AP if no credentials)
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect("FataMorgana-Setup");

  // Wait for WiFi connection
  Serial.print(F("Connecting to WiFi"));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    wifiManager.process();
  }
  Serial.println();
  Serial.print(F("Connected! IP: "));
  Serial.println(WiFi.localIP());

  // Initialize FataMorgana client
  if (!client.begin()) {
    Serial.println(F("ERROR: Failed to initialize FataMorgana client!"));
    while (1) delay(1000);
  }

  // Configure default mapping (Rectangle mode)
   client.setRowMapping(1, LED_COUNT);  // Row 5, all LEDs
  client.setSerpentine(SERPENTINE_HORIZONTAL);
  client.setRotation(0);
  client.setFlip(false, false, false);
  client.setBrightness(80);

  Serial.println(F("FataMorgana client ready"));

  // Start Web UI (Optional)
  if (!webUI.begin(80, 81)) {
    Serial.println(F("WARNING: Failed to start WebUI"));
  } else {
    Serial.print(F("Web interface: http://"));
    Serial.println(WiFi.localIP());
  }

  Serial.println(F("====================================="));
  Serial.println(F("Ready to receive frames!"));
  Serial.println(F("====================================="));

}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Process WiFi Manager
  wifiManager.process();

  // Process FataMorgana protocol (REQUIRED)
  client.loop();

  // Process Web UI (Optional - comment out if not using)
  webUI.loop();

  // Optional: Print status every 10 seconds
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus >= 10000) {
    lastStatus = millis();

    Serial.println(F("--- Status ---"));
    Serial.printf("Rendered: %u frames\n", client.getRenderedFrames());
    Serial.printf("Packets: %u accepted, %u rejected\n",
                  client.getAcceptedPackets(),
                  client.getRejectedPackets());
    Serial.printf("Last frame: %ux%u (%s)\n",
                  client.getLastFrameWidth(),
                  client.getLastFrameHeight(),
                  fatamorgana_rgbTypeName(client.getLastFrameRgbType()));
    Serial.println();
  }
}
