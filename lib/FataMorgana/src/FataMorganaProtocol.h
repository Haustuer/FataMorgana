/**
 * @file FataMorganaProtocol.h
 * @brief FataMorgana UDP Multicast Protocol Constants
 *
 * This file defines all protocol constants used by both the server
 * and client implementations of the FataMorgana LED display protocol.
 *
 * Protocol Version: 1.0
 */

#pragma once

#include <Arduino.h>
#include <IPAddress.h>

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

/** Multicast group address for server → device communication */
const IPAddress FATAMORGANA_MULTICAST_ADDR(239, 255, 42, 1);

/** UDP port for multicast frame distribution (server → devices) */
constexpr uint16_t FATAMORGANA_MULTICAST_PORT = 7777;

/** UDP port for unicast discovery responses (devices → server) */
constexpr uint16_t FATAMORGANA_RESPONSE_PORT = 7778;

// ============================================================================
// PACKET STRUCTURE
// ============================================================================

/** Size of packet header in bytes */
constexpr size_t FATAMORGANA_HEADER_SIZE = 8;

/** Maximum UDP packet size in bytes */
constexpr size_t FATAMORGANA_MAX_PACKET_SIZE = 1200;

/** Maximum payload size per packet (packet size - header) */
constexpr size_t FATAMORGANA_MAX_PAYLOAD_SIZE = FATAMORGANA_MAX_PACKET_SIZE - FATAMORGANA_HEADER_SIZE;

/** Maximum frame buffer size in bytes (supports up to ~2400 RGB332 pixels) */
constexpr size_t FATAMORGANA_MAX_FRAME_BYTES = 4800;

/** Maximum number of chunks per frame */
constexpr size_t FATAMORGANA_MAX_CHUNKS =
    (FATAMORGANA_MAX_FRAME_BYTES + FATAMORGANA_MAX_PAYLOAD_SIZE - 1) / FATAMORGANA_MAX_PAYLOAD_SIZE;

// ============================================================================
// FRAME TYPES
// ============================================================================

/** Frame Type: Configuration/control frame */
constexpr uint8_t FATAMORGANA_FRAME_TYPE_CONFIG = 0;

/** Frame Type: First packet of an image frame */
constexpr uint8_t FATAMORGANA_FRAME_TYPE_IMAGE_START = 1;

/** Frame Type: Subsequent packets of an image frame */
constexpr uint8_t FATAMORGANA_FRAME_TYPE_IMAGE_CONTINUATION = 2;

// ============================================================================
// CONFIG FRAME SUBTYPES
// ============================================================================

/** Config SubType: Discovery request from server */
constexpr uint8_t FATAMORGANA_CONFIG_SUBTYPE_DISCOVERY = 0;

/** Config SubType: Set device mapping configuration */
constexpr uint8_t FATAMORGANA_CONFIG_SUBTYPE_SET_MAPPING = 1;

/** Config SubType: Set device brightness */
constexpr uint8_t FATAMORGANA_CONFIG_SUBTYPE_SET_BRIGHTNESS = 2;

/** Config SubType: Identify device (flash LEDs) */
constexpr uint8_t FATAMORGANA_CONFIG_SUBTYPE_IDENTIFY = 3;

// ============================================================================
// RGB ENCODING TYPES
// ============================================================================

/** RGB Type: 8-bit color (3R 3G 2B) - 1 byte per pixel */
constexpr uint8_t FATAMORGANA_RGB332 = 0;

/** RGB Type: 16-bit color (5R 6G 5B) - 2 bytes per pixel, little-endian */
constexpr uint8_t FATAMORGANA_RGB565 = 1;

// ============================================================================
// DISCOVERY RESPONSE FORMAT
// ============================================================================

/** Size of binary discovery response packet */
constexpr size_t FATAMORGANA_DISCOVERY_RESPONSE_SIZE = 69;

/** Magic bytes for discovery response validation: "FATA" */
constexpr uint32_t FATAMORGANA_MAGIC_BYTES = 0x46415441;

/** Current protocol version */
constexpr uint8_t FATAMORGANA_PROTOCOL_VERSION = 1;

/** Discovery response type identifier */
constexpr uint8_t FATAMORGANA_RESPONSE_TYPE_DISCOVERY = 0x01;

// ============================================================================
// FIRMWARE VERSION
// ============================================================================

/** Library major version */
constexpr uint8_t FATAMORGANA_VERSION_MAJOR = 1;

/** Library minor version */
constexpr uint8_t FATAMORGANA_VERSION_MINOR = 0;

/** Library patch version */
constexpr uint8_t FATAMORGANA_VERSION_PATCH = 0;

// ============================================================================
// PROTOCOL HELPER FUNCTIONS
// ============================================================================

/**
 * Get the number of bytes per pixel for a given RGB type
 * @param rgbType RGB encoding type (RGB332 or RGB565)
 * @return Bytes per pixel (1 or 2), or 0 for unknown types
 */
inline size_t fatamorgana_bytesPerPixel(uint8_t rgbType) {
    switch (rgbType) {
        case FATAMORGANA_RGB332:
            return 1;
        case FATAMORGANA_RGB565:
            return 2;
        default:
            return 0;  // Unknown type
    }
}

/**
 * Calculate total frame size in bytes
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param rgbType RGB encoding type
 * @return Total frame size in bytes
 */
inline size_t fatamorgana_frameSize(uint16_t width, uint16_t height, uint8_t rgbType) {
    return static_cast<size_t>(width) * static_cast<size_t>(height) * fatamorgana_bytesPerPixel(rgbType);
}

/**
 * Calculate number of UDP packets needed for a frame
 * @param frameBytes Total frame size in bytes
 * @return Number of packets (chunks) needed
 */
inline uint8_t fatamorgana_chunkCount(size_t frameBytes) {
    if (frameBytes == 0) return 0;
    return static_cast<uint8_t>((frameBytes + FATAMORGANA_MAX_PAYLOAD_SIZE - 1) / FATAMORGANA_MAX_PAYLOAD_SIZE);
}

/**
 * Get human-readable name for RGB type
 * @param rgbType RGB encoding type
 * @return String name ("RGB332", "RGB565", or "unknown")
 */
inline const char* fatamorgana_rgbTypeName(uint8_t rgbType) {
    switch (rgbType) {
        case FATAMORGANA_RGB332:
            return "RGB332";
        case FATAMORGANA_RGB565:
            return "RGB565";
        default:
            return "unknown";
    }
}

/**
 * Get human-readable name for frame type
 * @param frameType Frame type constant
 * @return String name
 */
inline const char* fatamorgana_frameTypeName(uint8_t frameType) {
    switch (frameType) {
        case FATAMORGANA_FRAME_TYPE_CONFIG:
            return "config";
        case FATAMORGANA_FRAME_TYPE_IMAGE_START:
            return "image-start";
        case FATAMORGANA_FRAME_TYPE_IMAGE_CONTINUATION:
            return "image-continuation";
        default:
            return "unknown";
    }
}
