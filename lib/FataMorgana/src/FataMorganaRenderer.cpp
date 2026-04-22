/**
 * @file FataMorganaRenderer.cpp
 * @brief Implementation of FataMorgana Frame Rendering Engine
 */

#include "FataMorganaRenderer.h"

FataMorganaRenderer::FataMorganaRenderer(Adafruit_NeoPixel& strip)
    : _strip(strip),
      _frameBuffer(nullptr),
      _width(0),
      _height(0),
      _rgbType(FATAMORGANA_RGB332) {
}

void FataMorganaRenderer::clear(bool show) {
    const uint16_t ledCount = _strip.numPixels();
    for (uint16_t i = 0; i < ledCount; i++) {
        _strip.setPixelColor(i, 0, 0, 0);
    }
    if (show) {
        _strip.show();
    }
}

FataMorganaColor FataMorganaRenderer::decodePixel(size_t pixelIndex) const {
    if (_rgbType == FATAMORGANA_RGB332) {
        // RGB332: 3 bits red, 3 bits green, 2 bits blue
        const uint8_t packed = _frameBuffer[pixelIndex];
        return FataMorganaColor(
            ((packed >> 5) & 0x07) * 255 / 7,  // Red: 3 bits
            ((packed >> 2) & 0x07) * 255 / 7,  // Green: 3 bits
            (packed & 0x03) * 255 / 3           // Blue: 2 bits
        );
    }

    // RGB565: 5 bits red, 6 bits green, 5 bits blue (little-endian)
    const size_t offset = pixelIndex * 2;
    const uint16_t packed = static_cast<uint16_t>(_frameBuffer[offset]) |
                            (static_cast<uint16_t>(_frameBuffer[offset + 1]) << 8);
    return FataMorganaColor(
        ((packed >> 11) & 0x1F) * 255 / 31,  // Red: 5 bits
        ((packed >> 5) & 0x3F) * 255 / 63,   // Green: 6 bits
        (packed & 0x1F) * 255 / 31           // Blue: 5 bits
    );
}

FataMorganaColor FataMorganaRenderer::getPixelAt(int32_t x, int32_t y) const {
    if (x < 0 || y < 0 || x >= _width || y >= _height) {
        return FataMorganaColor(0, 0, 0);  // Black for out of bounds
    }
    return decodePixel(static_cast<size_t>(y) * _width + x);
}

FataMorganaColor FataMorganaRenderer::interpolate(const FataMorganaColor& left,
                                                   const FataMorganaColor& right,
                                                   float factor) const {
    return FataMorganaColor(
        static_cast<uint8_t>(left.r + (right.r - left.r) * factor + 0.5f),
        static_cast<uint8_t>(left.g + (right.g - left.g) * factor + 0.5f),
        static_cast<uint8_t>(left.b + (right.b - left.b) * factor + 0.5f)
    );
}

FataMorganaColor FataMorganaRenderer::sampleLine(bool rowMode,
                                                  uint16_t fixedIndex,
                                                  uint16_t ledIndex,
                                                  uint16_t totalLeds,
                                                  uint8_t sampleMode) const {
    const uint16_t sourceSpan = rowMode ? _width : _height;
    if (sourceSpan == 0) {
        return FataMorganaColor(0, 0, 0);
    }

    // Nearest neighbor sampling (or if interpolation not possible)
    if (sampleMode == FATAMORGANA_SAMPLE_PIXEL || sourceSpan == 1 || totalLeds == 1) {
        const uint32_t numerator = static_cast<uint32_t>(ledIndex) * static_cast<uint32_t>(sourceSpan - 1);
        const uint16_t sourceIndex = totalLeds <= 1
            ? 0
            : static_cast<uint16_t>((numerator + (totalLeds - 1) / 2) / (totalLeds - 1));
        return rowMode ? getPixelAt(sourceIndex, fixedIndex) : getPixelAt(fixedIndex, sourceIndex);
    }

    // Linear interpolation
    const float position = (static_cast<float>(ledIndex) * static_cast<float>(sourceSpan - 1)) /
                           static_cast<float>(totalLeds - 1);
    const uint16_t lowerIndex = static_cast<uint16_t>(position);
    uint16_t upperIndex = lowerIndex + 1;
    if (upperIndex >= sourceSpan) {
        upperIndex = sourceSpan - 1;
    }

    const float factor = position - lowerIndex;
    const FataMorganaColor lowerColor = rowMode
        ? getPixelAt(lowerIndex, fixedIndex)
        : getPixelAt(fixedIndex, lowerIndex);
    const FataMorganaColor upperColor = rowMode
        ? getPixelAt(upperIndex, fixedIndex)
        : getPixelAt(fixedIndex, upperIndex);

    return interpolate(lowerColor, upperColor, factor);
}

void FataMorganaRenderer::applySerpentine(uint16_t& row,
                                          uint16_t& column,
                                          uint16_t width,
                                          uint16_t height,
                                          uint8_t serpentine) const {
    if (serpentine == FATAMORGANA_SERPENTINE_HORIZONTAL && (row % 2 == 1)) {
        // Horizontal serpentine: reverse columns on odd rows (zigzag left-right)
        column = width - 1 - column;
    } else if (serpentine == FATAMORGANA_SERPENTINE_VERTICAL && (column % 2 == 1)) {
        // Vertical serpentine: reverse rows on odd columns (zigzag up-down)
        row = height - 1 - row;
    }
}

uint32_t FataMorganaRenderer::toNeoPixelColor(const FataMorganaColor& color) const {
    return _strip.Color(color.r, color.g, color.b);
}

void FataMorganaRenderer::renderFrame(const uint8_t* frameBuffer,
                                      uint16_t width,
                                      uint16_t height,
                                      uint8_t rgbType,
                                      const FataMorganaMapping& mapping) {
    // Clear strip first
    clear(false);

    // Store frame state
    _frameBuffer = frameBuffer;
    _width = width;
    _height = height;
    _rgbType = rgbType;

    // Check if frame is valid
    if (width == 0 || height == 0 || frameBuffer == nullptr) {
        _strip.show();
        return;
    }

    const uint16_t ledCount = _strip.numPixels();

    // Render based on mapping mode
    if (mapping.mode == FATAMORGANA_MAPPING_ROW) {
        // ROW MODE: Extract horizontal line
        if (mapping.rowIndex < height) {
            const uint16_t pixelsToRender = min(mapping.linePixels, ledCount);
            for (uint16_t ledIndex = 0; ledIndex < pixelsToRender; ledIndex++) {
                FataMorganaColor color = sampleLine(true, mapping.rowIndex, ledIndex,
                                                     mapping.linePixels, mapping.sampleMode);
                _strip.setPixelColor(ledIndex, toNeoPixelColor(color));
            }
        }
    } else if (mapping.mode == FATAMORGANA_MAPPING_COLUMN) {
        // COLUMN MODE: Extract vertical line
        if (mapping.columnIndex < width) {
            const uint16_t pixelsToRender = min(mapping.linePixels, ledCount);
            for (uint16_t ledIndex = 0; ledIndex < pixelsToRender; ledIndex++) {
                FataMorganaColor color = sampleLine(false, mapping.columnIndex, ledIndex,
                                                     mapping.linePixels, mapping.sampleMode);
                _strip.setPixelColor(ledIndex, toNeoPixelColor(color));
            }
        }
    } else {
        // RECTANGLE MODE: Extract rectangular region with transforms
        const size_t rectanglePixels = static_cast<size_t>(mapping.rectWidth) * mapping.rectHeight;
        const size_t pixelsToRender = rectanglePixels < ledCount ? rectanglePixels : ledCount;

        for (size_t ledIndex = 0; ledIndex < pixelsToRender; ledIndex++) {
            uint16_t row = static_cast<uint16_t>(ledIndex / mapping.rectWidth);
            if (row >= mapping.rectHeight) {
                break;
            }

            uint16_t column = static_cast<uint16_t>(ledIndex % mapping.rectWidth);

            // Apply serpentine pattern
            applySerpentine(row, column, mapping.rectWidth, mapping.rectHeight, mapping.serpentine);

            // Calculate source position
            const uint16_t sourceX = mapping.rectX + column;
            const uint16_t sourceY = mapping.rectY + row;

            // Get pixel color and set LED
            FataMorganaColor color = getPixelAt(sourceX, sourceY);
            _strip.setPixelColor(static_cast<uint16_t>(ledIndex), toNeoPixelColor(color));
        }
    }

    // Update LED strip
    _strip.show();
}
