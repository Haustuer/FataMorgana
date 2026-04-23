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

void FataMorganaRenderer::applyTransforms(uint16_t& row,
                                          uint16_t& column,
                                          uint16_t width,
                                          uint16_t height,
                                          uint8_t rotation,
                                          bool flipX,
                                          bool flipY,
                                          bool flipZ) const {
    // Apply diagonal flip (transpose) first
    if (flipZ) {
        uint16_t temp = row;
        row = column;
        column = temp;
        // Swap dimensions after transpose
        uint16_t tempDim = width;
        width = height;
        height = tempDim;
    }

    // Apply rotation
    switch (rotation) {
        case 1: // 90° clockwise
            {
                uint16_t newRow = column;
                uint16_t newColumn = height - 1 - row;
                row = newRow;
                column = newColumn;
                // Swap dimensions after 90° rotation
                uint16_t tempDim = width;
                width = height;
                height = tempDim;
            }
            break;
        case 2: // 180°
            row = height - 1 - row;
            column = width - 1 - column;
            break;
        case 3: // 270° clockwise (90° counter-clockwise)
            {
                uint16_t newRow = width - 1 - column;
                uint16_t newColumn = row;
                row = newRow;
                column = newColumn;
                // Swap dimensions after 270° rotation
                uint16_t tempDim = width;
                width = height;
                height = tempDim;
            }
            break;
        default: // 0° or invalid - no rotation
            break;
    }

    // Apply horizontal flip
    if (flipX) {
        column = width - 1 - column;
    }

    // Apply vertical flip
    if (flipY) {
        row = height - 1 - row;
    }
}

uint32_t FataMorganaRenderer::toNeoPixelColor(const FataMorganaColor& color, float gamma) const {
    // Apply gamma correction to each channel
    // Formula: corrected = pow(value / 255.0, gamma) * 255.0
    uint8_t r_corrected, g_corrected, b_corrected;

    if (gamma == 1.0f) {
        // No correction needed (linear)
        r_corrected = color.r;
        g_corrected = color.g;
        b_corrected = color.b;
    } else {
        // Apply gamma correction
        r_corrected = static_cast<uint8_t>(pow(color.r / 255.0f, gamma) * 255.0f + 0.5f);
        g_corrected = static_cast<uint8_t>(pow(color.g / 255.0f, gamma) * 255.0f + 0.5f);
        b_corrected = static_cast<uint8_t>(pow(color.b / 255.0f, gamma) * 255.0f + 0.5f);
    }

    return _strip.Color(r_corrected, g_corrected, b_corrected);
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
                _strip.setPixelColor(ledIndex, toNeoPixelColor(color, mapping.gamma));
            }
        }
    } else if (mapping.mode == FATAMORGANA_MAPPING_COLUMN) {
        // COLUMN MODE: Extract vertical line
        if (mapping.columnIndex < width) {
            const uint16_t pixelsToRender = min(mapping.linePixels, ledCount);
            for (uint16_t ledIndex = 0; ledIndex < pixelsToRender; ledIndex++) {
                FataMorganaColor color = sampleLine(false, mapping.columnIndex, ledIndex,
                                                     mapping.linePixels, mapping.sampleMode);
                _strip.setPixelColor(ledIndex, toNeoPixelColor(color, mapping.gamma));
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

            // Apply rotation and flip transforms
            applyTransforms(row, column, mapping.rectWidth, mapping.rectHeight,
                            mapping.rotation, mapping.flipX, mapping.flipY, mapping.flipZ);

            // Calculate source position
            const uint16_t sourceX = mapping.rectX + column;
            const uint16_t sourceY = mapping.rectY + row;

            // Get pixel color and set LED
            FataMorganaColor color = getPixelAt(sourceX, sourceY);
            _strip.setPixelColor(static_cast<uint16_t>(ledIndex), toNeoPixelColor(color, mapping.gamma));
        }
    }

    // Update LED strip
    _strip.show();
}
