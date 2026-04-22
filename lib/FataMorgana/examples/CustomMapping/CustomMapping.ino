/**
 * @file CustomMapping.ino
 * @brief Different mapping modes demonstration
 *
 * This example shows how to use different mapping modes:
 * - Rectangle mode (extract rectangular region)
 * - Row mode (extract horizontal line)
 * - Column mode (extract vertical line)
 *
 * Press 'r' in Serial Monitor to switch to rectangle mode
 * Press 'h' for horizontal row mode
 * Press 'v' for vertical column mode
 * Press 'i' for interpolated sampling (smooth)
 * Press 'p' for pixel sampling (fast)
 *
 * WiFi Setup:
 * - Uses WiFiManager for easy configuration
 * - On first boot, creates AP "FataMorgana-Mapping"
 * - Connect to AP and configure your WiFi credentials
 * - Credentials saved automatically, no code changes needed!
 *
 * Hardware:
 * - ESP8266 or ESP32
 * - WS2812B LED strip connected to GPIO 12
 * - 100 LEDs (adjustable)
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
#include <WiFiManager.h>  // For easy WiFi configuration
#include <FataMorgana.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

constexpr uint8_t LED_PIN = 12;
constexpr uint16_t LED_COUNT = 100;

// ============================================================================
// GLOBALS
// ============================================================================

WiFiManager wifiManager;
FataMorganaClient client(LED_COUNT, LED_PIN);

// Current mode tracking
uint8_t currentMode = MAPPING_RECTANGLE;
uint8_t currentSampleMode = SAMPLE_PIXEL;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void printCurrentConfig() {
  const FataMorganaMapping& mapping = client.getMapping();

  Serial.println(F("\n--- Current Configuration ---"));
  Serial.print(F("Mode: "));
  Serial.println(fatamorgana_mappingModeName(mapping.mode));

  if (mapping.mode == MAPPING_ROW) {
    Serial.printf("  Row: %u, Pixels: %u\n", mapping.rowIndex, mapping.linePixels);
    Serial.print(F("  Sample: "));
    Serial.println(fatamorgana_sampleModeName(mapping.sampleMode));
  } else if (mapping.mode == MAPPING_COLUMN) {
    Serial.printf("  Column: %u, Pixels: %u\n", mapping.columnIndex, mapping.linePixels);
    Serial.print(F("  Sample: "));
    Serial.println(fatamorgana_sampleModeName(mapping.sampleMode));
  } else {
    Serial.printf("  Rectangle: (%u,%u) %ux%u\n",
                  mapping.rectX, mapping.rectY,
                  mapping.rectWidth, mapping.rectHeight);
    Serial.print(F("  Serpentine: "));
    Serial.println(fatamorgana_serpentineModeName(mapping.serpentine));
  }
  Serial.println();
}

void setRectangleMode() {
  Serial.println(F("Switching to RECTANGLE mode..."));
  client.setRectangle(0, 0, 10, 10);
  client.setSerpentine(SERPENTINE_HORIZONTAL);
  currentMode = MAPPING_RECTANGLE;
  printCurrentConfig();
}

void setRowMode() {
  Serial.println(F("Switching to ROW mode..."));
  client.setRowMapping(5, LED_COUNT);  // Row 5, all LEDs
  client.setSampleMode(currentSampleMode);
  currentMode = MAPPING_ROW;
  printCurrentConfig();
}

void setColumnMode() {
  Serial.println(F("Switching to COLUMN mode..."));
  client.setColumnMapping(10, LED_COUNT);  // Column 10, all LEDs
  client.setSampleMode(currentSampleMode);
  currentMode = MAPPING_COLUMN;
  printCurrentConfig();
}

void setInterpolated() {
  Serial.println(F("Setting INTERPOLATED sampling..."));
  currentSampleMode = SAMPLE_INTERPOLATED;
  client.setSampleMode(SAMPLE_INTERPOLATED);
  printCurrentConfig();
}

void setPixel() {
  Serial.println(F("Setting PIXEL sampling..."));
  currentSampleMode = SAMPLE_PIXEL;
  client.setSampleMode(SAMPLE_PIXEL);
  printCurrentConfig();
}

void printHelp() {
  Serial.println(F("\n=== FataMorgana Mapping Demo ==="));
  Serial.println(F("Commands:"));
  Serial.println(F("  r - Rectangle mode (10x10 region)"));
  Serial.println(F("  h - Horizontal row mode (extract row 5)"));
  Serial.println(F("  v - Vertical column mode (extract column 10)"));
  Serial.println(F("  i - Interpolated sampling (smooth)"));
  Serial.println(F("  p - Pixel sampling (fast)"));
  Serial.println(F("  ? - Show this help"));
  Serial.println(F("================================\n"));
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println();

  printHelp();

  // Connect to WiFi using WiFiManager
  // On first run, creates AP "FataMorgana-Mapping" for configuration
  Serial.println(F("Connecting to WiFi..."));
  Serial.println(F("If not configured, connect to 'FataMorgana-Mapping' AP"));

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect("FataMorgana-Mapping");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    wifiManager.process();
  }

  Serial.println();
  Serial.print(F("Connected! IP: "));
  Serial.println(WiFi.localIP());

  // Initialize FataMorgana
  if (!client.begin()) {
    Serial.println(F("ERROR: Failed to initialize!"));
    while (1) delay(1000);
  }

  // Start with rectangle mode
  setRectangleMode();

  Serial.println(F("Ready! Press '?' for help."));
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Process WiFi Manager
  wifiManager.process();

  // Process FataMorgana protocol
  client.loop();

  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();  // Clear buffer

    switch (cmd) {
      case 'r':
      case 'R':
        setRectangleMode();
        break;

      case 'h':
      case 'H':
        setRowMode();
        break;

      case 'v':
      case 'V':
        setColumnMode();
        break;

      case 'i':
      case 'I':
        setInterpolated();
        break;

      case 'p':
      case 'P':
        setPixel();
        break;

      case '?':
        printHelp();
        printCurrentConfig();
        break;

      default:
        Serial.println(F("Unknown command. Press '?' for help."));
        break;
    }
  }
}
