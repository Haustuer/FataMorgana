/**
 * @file BasicClient.ino
 * @brief Minimal FataMorgana client example
 *
 * This example shows the absolute minimum code needed to use
 * FataMorgana. Just connect to WiFi and start receiving frames!
 *
 * Hardware:
 * - ESP8266 or ESP32
 * - WS2812B LED strip connected to GPIO 12
 * - 256 LEDs (adjustable)
 *
 * @author FataMorgana
 * @date 2026
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>  // Use <WiFi.h> for ESP32
#include <FataMorgana.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// WiFi credentials
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// LED configuration
constexpr uint8_t LED_PIN = 12;
constexpr uint16_t LED_COUNT = 256;

// ============================================================================
// GLOBALS
// ============================================================================

FataMorganaClient client(LED_COUNT, LED_PIN);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("FataMorgana Basic Client"));
  Serial.println(F("========================"));

  // Connect to WiFi
  Serial.print(F("Connecting to WiFi"));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println();
  Serial.print(F("Connected! IP: "));
  Serial.println(WiFi.localIP());

  // Initialize FataMorgana
  if (!client.begin()) {
    Serial.println(F("ERROR: Failed to initialize!"));
    while (1) delay(1000);
  }

  // Configure mapping - 16x16 rectangle at origin
  client.setRectangle(0, 0, 16, 16);
  client.setBrightness(80);

  Serial.println(F("Ready to receive frames!"));
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Process incoming UDP packets
  client.loop();
}
