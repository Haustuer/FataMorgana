#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>

// ===== CONFIG =====
#define LED_PIN 12
#define LED_COUNT 100
#define UDP_PORT 7777

WiFiUDP udp;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t packetBuffer[400]; // enough for 100 LEDs

WiFiManager wm;

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.show();
  strip.setBrightness(80);

  // WiFiManager non-blocking mode
  wm.setConfigPortalBlocking(false);
 // wm.setConfigPortalTimeout(60);  // optional
  wm.autoConnect("LED-Setup");    // starts AP if needed

  udp.begin(UDP_PORT);
  Serial.printf("UDP listening on %d\n", UDP_PORT);
}

void loop() {
  // Keep WiFiManager alive
  wm.process();

  // Handle UDP packets
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    int len = udp.read(packetBuffer, sizeof(packetBuffer));

    if (len >= 2 && packetBuffer[0] == 0xAA) {
      int count = packetBuffer[1];
      if (count > LED_COUNT) count = LED_COUNT;

      int offset = 2;
      for (int i = 0; i < count; i++) {
        uint8_t r = packetBuffer[offset++];
        uint8_t g = packetBuffer[offset++];
        uint8_t b = packetBuffer[offset++];
        strip.setPixelColor(i, r, g, b);
      }
      strip.show();
    }
  }
}