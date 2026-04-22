/**
 * @file FataMorganaClient.h
 * @brief FataMorgana Client - Main library interface
 *
 * This is the primary class users interact with. It handles UDP
 * multicast reception, frame assembly, discovery responses, and
 * delegates rendering to FataMorganaRenderer.
 */

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include "FataMorganaProtocol.h"
#include "FataMorganaConfig.h"
#include "FataMorganaRenderer.h"

/**
 * @brief Main FataMorgana client class
 *
 * Handles UDP multicast reception, frame assembly, protocol handling,
 * and LED rendering. This is the main API users interact with.
 */
class FataMorganaClient {
public:
    /**
     * Constructor
     * @param ledCount Number of LEDs in the strip
     * @param ledPin GPIO pin for LED data
     * @param ledType NeoPixel LED type (default: NEO_GRB + NEO_KHZ800)
     */
    FataMorganaClient(uint16_t ledCount, uint8_t ledPin, uint8_t ledType = NEO_GRB + NEO_KHZ800);

    /**
     * Destructor
     */
    ~FataMorganaClient();

    /**
     * Initialize the client (call in setup())
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Process incoming UDP packets (call in loop())
     */
    void loop();

    // ========================================================================
    // Configuration Methods
    // ========================================================================

    /**
     * Set rectangle mapping mode
     * @param x Rectangle X offset in source image
     * @param y Rectangle Y offset in source image
     * @param width Rectangle width
     * @param height Rectangle height
     */
    void setRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    /**
     * Set row mapping mode
     * @param rowIndex Row index to extract
     * @param pixels Number of LEDs to map
     */
    void setRowMapping(uint16_t rowIndex, uint16_t pixels);

    /**
     * Set column mapping mode
     * @param columnIndex Column index to extract
     * @param pixels Number of LEDs to map
     */
    void setColumnMapping(uint16_t columnIndex, uint16_t pixels);

    /**
     * Set sample mode for row/column mapping
     * @param mode SAMPLE_PIXEL or SAMPLE_INTERPOLATED
     */
    void setSampleMode(uint8_t mode);

    /**
     * Set serpentine layout mode (rectangle mode only)
     * @param mode SERPENTINE_NONE, SERPENTINE_HORIZONTAL, or SERPENTINE_VERTICAL
     */
    void setSerpentine(uint8_t mode);

    /**
     * Set rotation (rectangle mode only)
     * @param rotation 0=0°, 1=90°, 2=180°, 3=270°
     */
    void setRotation(uint8_t rotation);

    /**
     * Set flip transforms (rectangle mode only)
     * @param x Flip horizontally
     * @param y Flip vertically
     * @param z Flip diagonally (transpose)
     */
    void setFlip(bool x, bool y, bool z);

    /**
     * Set LED brightness
     * @param brightness 0-255
     */
    void setBrightness(uint8_t brightness);

    /**
     * Get current mapping configuration
     * @return Reference to mapping configuration
     */
    const FataMorganaMapping& getMapping() const { return _mapping; }

    // ========================================================================
    // Status Methods
    // ========================================================================

    /**
     * Get number of rendered frames
     * @return Rendered frame count
     */
    uint32_t getRenderedFrames() const { return _renderedFrames; }

    /**
     * Get number of accepted packets
     * @return Accepted packet count
     */
    uint32_t getAcceptedPackets() const { return _acceptedPackets; }

    /**
     * Get number of rejected packets
     * @return Rejected packet count
     */
    uint32_t getRejectedPackets() const { return _rejectedPackets; }

    /**
     * Check if frame assembly is in progress
     * @return true if receiving frame chunks
     */
    bool isFrameInProgress() const { return _frameInProgress; }

    /**
     * Get last frame width
     * @return Width in pixels
     */
    uint16_t getLastFrameWidth() const { return _lastFrameWidth; }

    /**
     * Get last frame height
     * @return Height in pixels
     */
    uint16_t getLastFrameHeight() const { return _lastFrameHeight; }

    /**
     * Get last frame RGB type
     * @return RGB type (RGB332 or RGB565)
     */
    uint8_t getLastFrameRgbType() const { return _lastFrameRgbType; }

    /**
     * Get device uptime in milliseconds
     * @return Uptime in ms
     */
    unsigned long getUptime() const { return millis(); }

    /**
     * Get access to the LED strip
     * @return Reference to NeoPixel strip
     */
    Adafruit_NeoPixel& getStrip() { return _strip; }

    /**
     * Re-render the last received frame with current settings
     * Useful for immediately seeing effect of configuration changes
     * @return true if frame was re-rendered, false if no frame stored
     */
    bool reRenderLastFrame();

private:
    // Hardware
    Adafruit_NeoPixel _strip;
    FataMorganaRenderer _renderer;
    uint16_t _ledCount;
    uint8_t _ledPin;

    // Network
    WiFiUDP _udp;

    // Configuration
    FataMorganaMapping _mapping;

    // Frame assembly state
    bool _frameInProgress;
    uint8_t _activeFrameCounter;
    uint8_t _activeRgbType;
    uint16_t _activeWidth;
    uint16_t _activeHeight;
    size_t _activeFrameBytes;
    uint8_t _expectedChunkCount;
    uint8_t _receivedChunkCount;
    uint8_t* _frameBuffer;
    bool* _chunkReceived;

    // Stored frame for re-rendering
    uint8_t* _storedFrameBuffer;      // Copy of last complete frame
    size_t _storedFrameSize;          // Size of stored frame
    uint16_t _storedFrameWidth;       // Stored frame width
    uint16_t _storedFrameHeight;      // Stored frame height
    uint8_t _storedFrameRgbType;      // Stored frame RGB type
    bool _hasStoredFrame;             // Do we have a frame stored?

    // Statistics
    uint32_t _acceptedPackets;
    uint32_t _rejectedPackets;
    uint32_t _renderedFrames;
    uint16_t _lastFrameWidth;
    uint16_t _lastFrameHeight;
    uint8_t _lastFrameRgbType;
    unsigned long _lastPacketMillis;
    unsigned long _lastRenderMillis;

    // Packet buffer
    uint8_t _packetBuffer[FATAMORGANA_MAX_PACKET_SIZE];

    /**
     * Handle incoming UDP packet
     * @param data Packet data
     * @param length Packet length
     */
    void handlePacket(const uint8_t* data, size_t length);

    /**
     * Handle discovery request
     * @param serverIP Server IP address
     * @param serverPort Server response port
     */
    void handleDiscoveryRequest(IPAddress serverIP, uint16_t serverPort);

    /**
     * Send binary discovery response
     * @param serverIP Server IP address
     * @param serverPort Server response port
     */
    void sendDiscoveryResponse(IPAddress serverIP, uint16_t serverPort);

    /**
     * Begin frame assembly
     * @param frameCounter Frame counter
     * @param rgbType RGB encoding type
     * @param width Frame width
     * @param height Frame height
     * @return true if frame assembly started
     */
    bool beginFrame(uint8_t frameCounter, uint8_t rgbType, uint16_t width, uint16_t height);

    /**
     * Validate active frame parameters
     * @param frameCounter Frame counter
     * @param rgbType RGB type
     * @param width Frame width
     * @param height Frame height
     * @return true if parameters match active frame
     */
    bool validateActiveFrame(uint8_t frameCounter, uint8_t rgbType, uint16_t width, uint16_t height);

    /**
     * Store frame chunk
     * @param chunkIndex Chunk index
     * @param payload Chunk payload data
     * @param payloadLength Payload length
     * @return true if chunk stored successfully
     */
    bool storeChunk(uint8_t chunkIndex, const uint8_t* payload, size_t payloadLength);

    /**
     * Finalize and render complete frame
     */
    void renderCompleteFrame();
};
