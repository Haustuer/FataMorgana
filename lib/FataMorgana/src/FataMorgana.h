/**
 * @file FataMorgana.h
 * @brief Main header for FataMorgana LED Display Library
 *
 * Include this single header to use the FataMorgana library.
 *
 * @mainpage FataMorgana LED Display Library
 *
 * @section intro Introduction
 *
 * FataMorgana is a high-performance UDP multicast protocol library for
 * distributed LED display systems. It allows you to control multiple
 * ESP8266/ESP32 devices with synchronized image frames, advanced mapping
 * modes, and real-time transform capabilities.
 *
 * @section features Features
 *
 * - UDP Multicast Protocol for efficient one-to-many distribution
 * - Multiple Mapping Modes (Row, Column, Rectangle extraction)
 * - Advanced Transforms (Rotation, Flip X/Y/Z)
 * - Serpentine Layouts (Horizontal and Vertical LED wiring patterns)
 * - Dual RGB Encoding (RGB332 and RGB565)
 * - Optional Web Interface with WebSocket real-time updates
 * - Auto-Discovery protocol
 * - High Performance (optimized for ESP8266/ESP32)
 * - Interpolated Sampling for smooth scaling
 *
 * @section quick_start Quick Start
 *
 * @code
 * #include <FataMorgana.h>
 *
 * FataMorganaClient client(256, 12);  // 256 LEDs on pin 12
 *
 * void setup() {
 *   WiFi.begin("SSID", "PASSWORD");
 *   while (WiFi.status() != WL_CONNECTED) delay(500);
 *
 *   client.begin();
 *   client.setRectangle(0, 0, 16, 16);
 *   client.setSerpentine(FATAMORGANA_SERPENTINE_HORIZONTAL);
 * }
 *
 * void loop() {
 *   client.loop();
 * }
 * @endcode
 *
 * @section web_ui Web Interface (Optional)
 *
 * @code
 * #include <FataMorgana.h>
 * #include <FataMorganaWebUI.h>
 *
 * FataMorganaClient client(256, 12);
 * FataMorganaWebUI webUI(client);
 *
 * void setup() {
 *   WiFi.begin("SSID", "PASSWORD");
 *   client.begin();
 *   webUI.begin();  // Starts HTTP server on port 80, WebSocket on port 81
 * }
 *
 * void loop() {
 *   client.loop();
 *   webUI.loop();
 * }
 * @endcode
 *
 * @section license License
 *
 * MIT License - See LICENSE file for details
 *
 * @author Your Name
 * @version 1.0.0
 * @date 2026
 */

#pragma once

// Core protocol definitions
#include "FataMorganaProtocol.h"

// Configuration structures and types
#include "FataMorganaConfig.h"

// Rendering engine
#include "FataMorganaRenderer.h"

// Main client class
#include "FataMorganaClient.h"

// Note: For web configuration interface, install the separate
// FataMorgana-WebUI library and include <FataMorganaWebUI.h>

/**
 * @namespace Compatibility Aliases
 *
 * For convenience, provide shorter aliases for common constants
 */

// Mapping modes
#define MAPPING_ROW FATAMORGANA_MAPPING_ROW
#define MAPPING_COLUMN FATAMORGANA_MAPPING_COLUMN
#define MAPPING_RECTANGLE FATAMORGANA_MAPPING_RECTANGLE

// Sample modes
#define SAMPLE_PIXEL FATAMORGANA_SAMPLE_PIXEL
#define SAMPLE_INTERPOLATED FATAMORGANA_SAMPLE_INTERPOLATED

// Serpentine modes
#define SERPENTINE_NONE FATAMORGANA_SERPENTINE_NONE
#define SERPENTINE_HORIZONTAL FATAMORGANA_SERPENTINE_HORIZONTAL
#define SERPENTINE_VERTICAL FATAMORGANA_SERPENTINE_VERTICAL

// RGB types
#define RGB332 FATAMORGANA_RGB332
#define RGB565 FATAMORGANA_RGB565

// Type aliases for convenience
using FataMorganaColor = FataMorganaColor;
using FataMorganaMapping = FataMorganaMapping;

/**
 * @brief Print library version and information
 */
inline void fatamorgana_printVersion() {
    Serial.println(F("====================================="));
    Serial.print(F("FataMorgana LED Display Library v"));
    Serial.print(FATAMORGANA_VERSION_MAJOR);
    Serial.print(F("."));
    Serial.print(FATAMORGANA_VERSION_MINOR);
    Serial.print(F("."));
    Serial.println(FATAMORGANA_VERSION_PATCH);
    Serial.println(F("UDP Multicast Protocol for Distributed LEDs"));
    Serial.println(F("====================================="));
}
