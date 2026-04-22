/**
 * @file FataMorganaRenderer.h
 * @brief FataMorgana Frame Rendering Engine
 *
 * This class handles all frame decoding and LED rendering logic,
 * including mapping modes, transforms, and color interpolation.
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "FataMorganaProtocol.h"
#include "FataMorganaConfig.h"

/**
 * @brief Frame rendering engine for FataMorgana
 *
 * The renderer takes raw frame data and applies mapping modes,
 * transforms (rotation/flips), serpentine patterns, and color
 * sampling to output pixels to an LED strip.
 */
class FataMorganaRenderer {
public:
    /**
     * Constructor
     * @param strip Reference to Adafruit_NeoPixel strip
     */
    FataMorganaRenderer(Adafruit_NeoPixel& strip);

    /**
     * Render a complete frame to the LED strip
     * @param frameBuffer Raw frame data (RGB332 or RGB565)
     * @param width Frame width in pixels
     * @param height Frame height in pixels
     * @param rgbType RGB encoding type (RGB332 or RGB565)
     * @param mapping Mapping configuration to use
     */
    void renderFrame(const uint8_t* frameBuffer,
                     uint16_t width,
                     uint16_t height,
                     uint8_t rgbType,
                     const FataMorganaMapping& mapping);

    /**
     * Clear the LED strip (all LEDs off)
     * @param show If true, immediately update the strip
     */
    void clear(bool show = true);

private:
    Adafruit_NeoPixel& _strip;  ///< Reference to LED strip

    // Current frame state
    const uint8_t* _frameBuffer;  ///< Pointer to current frame data
    uint16_t _width;              ///< Current frame width
    uint16_t _height;             ///< Current frame height
    uint8_t _rgbType;             ///< Current RGB encoding type

    /**
     * Decode a single pixel from frame buffer
     * @param pixelIndex Pixel index in frame buffer
     * @return Decoded RGB color
     */
    FataMorganaColor decodePixel(size_t pixelIndex) const;

    /**
     * Get pixel color from frame at specified coordinates
     * @param x X coordinate (0-based)
     * @param y Y coordinate (0-based)
     * @return RGB color (black if out of bounds)
     */
    FataMorganaColor getPixelAt(int32_t x, int32_t y) const;

    /**
     * Sample a line (row or column) with optional interpolation
     * @param rowMode True for row sampling, false for column sampling
     * @param fixedIndex Fixed row/column index
     * @param ledIndex LED position along the line
     * @param totalLeds Total number of LEDs in the line
     * @param sampleMode Sampling mode (PIXEL or INTERPOLATED)
     * @return Sampled RGB color
     */
    FataMorganaColor sampleLine(bool rowMode,
                                 uint16_t fixedIndex,
                                 uint16_t ledIndex,
                                 uint16_t totalLeds,
                                 uint8_t sampleMode) const;

    /**
     * Interpolate between two colors
     * @param left First color
     * @param right Second color
     * @param factor Interpolation factor (0.0 = left, 1.0 = right)
     * @return Interpolated color
     */
    FataMorganaColor interpolate(const FataMorganaColor& left,
                                  const FataMorganaColor& right,
                                  float factor) const;

    /**
     * Apply serpentine pattern transformation
     * @param row Row index
     * @param column Column index
     * @param width Rectangle width
     * @param height Rectangle height
     * @param serpentine Serpentine mode
     */
    void applySerpentine(uint16_t& row,
                         uint16_t& column,
                         uint16_t width,
                         uint16_t height,
                         uint8_t serpentine) const;

    /**
     * Apply rotation and flip transformations
     * @param row Row index
     * @param column Column index
     * @param width Rectangle width
     * @param height Rectangle height
     * @param rotation Rotation (0=0°, 1=90°, 2=180°, 3=270°)
     * @param flipX Horizontal flip
     * @param flipY Vertical flip
     * @param flipZ Diagonal flip (transpose)
     */
    void applyTransforms(uint16_t& row,
                         uint16_t& column,
                         uint16_t width,
                         uint16_t height,
                         uint8_t rotation,
                         bool flipX,
                         bool flipY,
                         bool flipZ) const;

    /**
     * Convert FataMorganaColor to NeoPixel format
     * @param color RGB color
     * @return 32-bit NeoPixel color value
     */
    uint32_t toNeoPixelColor(const FataMorganaColor& color) const;
};
