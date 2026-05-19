/**
 * @file FataMorganaConfig.h
 * @brief FataMorgana Configuration Structures and Types
 *
 * This file defines all configuration structures, enums, and helper
 * functions used by the FataMorgana LED display library.
 */

#pragma once

#include <Arduino.h>

// ============================================================================
// MAPPING MODES
// ============================================================================

/** Mapping Mode: Extract a horizontal row from the image */
constexpr uint8_t FATAMORGANA_MAPPING_ROW = 0;

/** Mapping Mode: Extract a vertical column from the image */
constexpr uint8_t FATAMORGANA_MAPPING_COLUMN = 1;

/** Mapping Mode: Extract a rectangular region from the image */
constexpr uint8_t FATAMORGANA_MAPPING_RECTANGLE = 2;

// ============================================================================
// SAMPLE MODES
// ============================================================================

/** Sample Mode: Nearest neighbor pixel sampling (fast) */
constexpr uint8_t FATAMORGANA_SAMPLE_PIXEL = 0;

/** Sample Mode: Linear interpolation between pixels (smooth) */
constexpr uint8_t FATAMORGANA_SAMPLE_INTERPOLATED = 1;

// ============================================================================
// SERPENTINE MODES
// ============================================================================

/** Serpentine Mode: No serpentine layout (normal left-to-right) */
constexpr uint8_t FATAMORGANA_SERPENTINE_NONE = 0;

/** Serpentine Mode: Horizontal serpentine (zigzag across rows, reverses columns on odd rows) */
constexpr uint8_t FATAMORGANA_SERPENTINE_HORIZONTAL = 1;

/** Serpentine Mode: Vertical serpentine (zigzag down columns, reverses rows on odd columns) */
constexpr uint8_t FATAMORGANA_SERPENTINE_VERTICAL = 2;

// ============================================================================
// OUT-OF-BOUNDS MODES
// ============================================================================

/** OOB Mode: Return black (0,0,0) for out-of-bounds pixels */
constexpr uint8_t FATAMORGANA_OOB_BLACK = 0;

/** OOB Mode: Clamp to edge (hold/repeat border pixel) */
constexpr uint8_t FATAMORGANA_OOB_CLAMP = 1;

/** OOB Mode: Mirror/reflect at boundaries */
constexpr uint8_t FATAMORGANA_OOB_MIRROR = 2;

// ============================================================================
// RGB COLOR STRUCTURE
// ============================================================================

/**
 * @brief Simple RGB color structure
 */
struct FataMorganaColor {
    uint8_t r;  ///< Red component (0-255)
    uint8_t g;  ///< Green component (0-255)
    uint8_t b;  ///< Blue component (0-255)

    /** Default constructor - creates black color */
    FataMorganaColor() : r(0), g(0), b(0) {}

    /** Constructor with RGB values */
    FataMorganaColor(uint8_t red, uint8_t green, uint8_t blue)
        : r(red), g(green), b(blue) {}
};

// ============================================================================
// MAPPING CONFIGURATION
// ============================================================================

/**
 * @brief Complete mapping configuration for LED strip
 *
 * This structure contains all parameters that define how the incoming
 * image frame is mapped onto the physical LED strip.
 */
struct FataMorganaMapping {
    // Basic mapping mode
    uint8_t mode;                ///< Mapping mode (ROW, COLUMN, or RECTANGLE)
    uint8_t sampleMode;          ///< Sample mode (PIXEL or INTERPOLATED)

    // Row/Column mode parameters
    uint16_t rowIndex;           ///< Row index for ROW mode
    uint16_t columnIndex;        ///< Column index for COLUMN mode
    uint16_t linePixels;         ///< Number of LEDs for ROW/COLUMN mode

    // Rectangle mode parameters
    uint16_t rectX;              ///< Rectangle X offset in source image
    uint16_t rectY;              ///< Rectangle Y offset in source image
    uint16_t rectWidth;          ///< Rectangle width in pixels
    uint16_t rectHeight;         ///< Rectangle height in pixels

    // Layout and transform options
    uint8_t serpentine;          ///< Serpentine layout mode (NONE, HORIZONTAL, VERTICAL)
    uint8_t rotation;            ///< Rotation (0=0°, 1=90°, 2=180°, 3=270°)
    bool flipX;                  ///< Flip horizontally (mirror across vertical axis)
    bool flipY;                  ///< Flip vertically (mirror across horizontal axis)
    bool flipZ;                  ///< Flip diagonally (transpose/swap X and Y)

    // Out-of-bounds handling
    uint8_t oobMode;             ///< Out-of-bounds mode (BLACK, CLAMP, or MIRROR)

    // Color correction
    float gamma;                 ///< Gamma correction (1.0=linear, 2.2=standard, 2.8=typical LEDs)

    /** Default constructor - initializes to rectangle mode at origin */
    FataMorganaMapping()
        : mode(FATAMORGANA_MAPPING_RECTANGLE),
          sampleMode(FATAMORGANA_SAMPLE_PIXEL),
          rowIndex(0),
          columnIndex(0),
          linePixels(100),
          rectX(0),
          rectY(0),
          rectWidth(10),
          rectHeight(10),
          serpentine(FATAMORGANA_SERPENTINE_NONE),
          rotation(0),
          flipX(false),
          flipY(false),
          flipZ(false),
          oobMode(FATAMORGANA_OOB_BLACK),
          gamma(2.2f) {}
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Get human-readable name for mapping mode
 * @param mode Mapping mode constant
 * @return String name ("row", "column", or "rectangle")
 */
inline const char* fatamorgana_mappingModeName(uint8_t mode) {
    switch (mode) {
        case FATAMORGANA_MAPPING_ROW:
            return "row";
        case FATAMORGANA_MAPPING_COLUMN:
            return "column";
        case FATAMORGANA_MAPPING_RECTANGLE:
            return "rectangle";
        default:
            return "unknown";
    }
}

/**
 * Get human-readable name for sample mode
 * @param mode Sample mode constant
 * @return String name ("pixel" or "interpolated")
 */
inline const char* fatamorgana_sampleModeName(uint8_t mode) {
    switch (mode) {
        case FATAMORGANA_SAMPLE_PIXEL:
            return "pixel";
        case FATAMORGANA_SAMPLE_INTERPOLATED:
            return "interpolated";
        default:
            return "unknown";
    }
}

/**
 * Get human-readable name for serpentine mode
 * @param mode Serpentine mode constant
 * @return String name ("none", "horizontal", or "vertical")
 */
inline const char* fatamorgana_serpentineModeName(uint8_t mode) {
    switch (mode) {
        case FATAMORGANA_SERPENTINE_NONE:
            return "none";
        case FATAMORGANA_SERPENTINE_HORIZONTAL:
            return "horizontal";
        case FATAMORGANA_SERPENTINE_VERTICAL:
            return "vertical";
        default:
            return "unknown";
    }
}

/**
 * Get human-readable name for out-of-bounds mode
 * @param mode OOB mode constant
 * @return String name ("black", "clamp", or "mirror")
 */
inline const char* fatamorgana_oobModeName(uint8_t mode) {
    switch (mode) {
        case FATAMORGANA_OOB_BLACK:
            return "black";
        case FATAMORGANA_OOB_CLAMP:
            return "clamp";
        case FATAMORGANA_OOB_MIRROR:
            return "mirror";
        default:
            return "unknown";
    }
}

/**
 * Sanitize mapping configuration to valid ranges
 * @param config Mapping configuration to validate/fix
 * @param ledCount Total number of LEDs available
 */
inline void fatamorgana_sanitizeMapping(FataMorganaMapping& config, uint16_t ledCount) {
    // Validate mapping mode
    if (config.mode > FATAMORGANA_MAPPING_RECTANGLE) {
        config.mode = FATAMORGANA_MAPPING_RECTANGLE;
    }

    // Validate sample mode
    if (config.sampleMode > FATAMORGANA_SAMPLE_INTERPOLATED) {
        config.sampleMode = FATAMORGANA_SAMPLE_PIXEL;
    }

    // Validate line pixels
    if (config.linePixels < 1) {
        config.linePixels = 1;
    }
    if (config.linePixels > ledCount) {
        config.linePixels = ledCount;
    }

    // Validate rectangle dimensions
    if (config.rectWidth < 1) {
        config.rectWidth = 1;
    }
    if (config.rectHeight < 1) {
        config.rectHeight = 1;
    }

    // Validate rotation (0-3 for 0°/90°/180°/270°)
    if (config.rotation > 3) {
        config.rotation = 0;
    }

    // Validate serpentine mode
    if (config.serpentine > FATAMORGANA_SERPENTINE_VERTICAL) {
        config.serpentine = FATAMORGANA_SERPENTINE_NONE;
    }

    // Validate out-of-bounds mode
    if (config.oobMode > FATAMORGANA_OOB_MIRROR) {
        config.oobMode = FATAMORGANA_OOB_BLACK;
    }

    // Validate gamma (1.0 to 3.5 is reasonable range)
    if (config.gamma < 1.0f) {
        config.gamma = 1.0f;
    }
    if (config.gamma > 3.5f) {
        config.gamma = 3.5f;
    }
}

/**
 * Build human-readable mapping summary string
 * @param config Mapping configuration
 * @param buffer Output buffer
 * @param bufferSize Size of output buffer
 */
inline void fatamorgana_mappingSummary(const FataMorganaMapping& config, char* buffer, size_t bufferSize) {
    if (config.mode == FATAMORGANA_MAPPING_ROW) {
        snprintf(buffer, bufferSize,
                 "row %u, %u leds, %s",
                 config.rowIndex,
                 config.linePixels,
                 fatamorgana_sampleModeName(config.sampleMode));
    } else if (config.mode == FATAMORGANA_MAPPING_COLUMN) {
        snprintf(buffer, bufferSize,
                 "column %u, %u leds, %s",
                 config.columnIndex,
                 config.linePixels,
                 fatamorgana_sampleModeName(config.sampleMode));
    } else {
        snprintf(buffer, bufferSize,
                 "rect %u,%u %ux%u, serpentine %s",
                 config.rectX,
                 config.rectY,
                 config.rectWidth,
                 config.rectHeight,
                 fatamorgana_serpentineModeName(config.serpentine));
    }
}
