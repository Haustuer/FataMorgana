# FataMorgana API Reference

Complete API documentation for the FataMorgana LED display library system.

---

## Table of Contents

- [FataMorganaClient](#fatamorganaclient) - Core protocol client
- [FataMorganaWebUI](#fatamorganawebui) - Optional web interface
- [FataMorganaMapping](#fatamorganamapping) - Configuration structure
- [Protocol Constants](#protocol-constants)
- [Helper Functions](#helper-functions)

---

## FataMorganaClient

Main client class for receiving and rendering FataMorgana frames.

### Constructor

```cpp
FataMorganaClient(uint16_t ledCount, uint8_t ledPin, uint8_t ledType = NEO_GRB + NEO_KHZ800)
```

**Parameters:**
- `ledCount` - Total number of LEDs in your strip/matrix
- `ledPin` - GPIO pin connected to LED data line (e.g., 12)
- `ledType` - NeoPixel type constant (default: `NEO_GRB + NEO_KHZ800`)

**Common LED Types:**
- `NEO_GRB + NEO_KHZ800` - WS2812/WS2812B (most common)
- `NEO_RGB + NEO_KHZ800` - WS2811
- `NEO_GRBW + NEO_KHZ800` - SK6812 RGBW

**Example:**
```cpp
FataMorganaClient client(256, 12);  // 256 LEDs on GPIO 12
```

---

### Lifecycle Methods

#### `bool begin()`

Initialize the client and join multicast group.

**Returns:** `true` on success, `false` on failure

**Must be called:** After WiFi connection, before `loop()`

**Example:**
```cpp
void setup() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  if (!client.begin()) {
    Serial.println("Failed to initialize!");
  }
}
```

---

#### `void loop()`

Process incoming UDP packets and render frames.

**Must be called:** Continuously in your main loop

**Example:**
```cpp
void loop() {
  client.loop();
}
```

---

### Mapping Configuration

#### `void setRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height)`

Extract a rectangular region from the image.

**Parameters:**
- `x` - Left edge coordinate (0-based)
- `y` - Top edge coordinate (0-based)
- `width` - Rectangle width in pixels
- `height` - Rectangle height in pixels

**Requirements:**
- `width * height` must equal your LED count
- Coordinates must be within frame boundaries

**Example:**
```cpp
// 16x16 matrix starting at top-left corner
client.setRectangle(0, 0, 16, 16);

// 8x25 region offset from top
client.setRectangle(4, 10, 8, 25);
```

---

#### `void setRowMapping(uint16_t rowIndex, uint16_t pixels)`

Extract a horizontal line from the image.

**Parameters:**
- `rowIndex` - Y coordinate of the row to extract
- `pixels` - Number of pixels to sample (must equal LED count)

**Use case:** Horizontal LED strips that span wider than the source image

**Example:**
```cpp
// Sample row 5 across 100 LEDs
client.setRowMapping(5, 100);
```

---

#### `void setColumnMapping(uint16_t columnIndex, uint16_t pixels)`

Extract a vertical line from the image.

**Parameters:**
- `columnIndex` - X coordinate of the column to extract
- `pixels` - Number of pixels to sample (must equal LED count)

**Use case:** Vertical LED strips that span taller than the source image

**Example:**
```cpp
// Sample column 10 across 100 LEDs
client.setColumnMapping(10, 100);
```

---

#### `void setSampleMode(uint8_t sampleMode)`

Set sampling mode for row/column mappings.

**Parameters:**
- `sampleMode` - `SAMPLE_PIXEL` or `SAMPLE_INTERPOLATED`

**Modes:**
- `SAMPLE_PIXEL` - Nearest-neighbor sampling (fast, blocky)
- `SAMPLE_INTERPOLATED` - Bilinear interpolation (smooth, slower)

**Only applies to:** Row and column modes (not rectangle)

**Example:**
```cpp
client.setColumnMapping(10, 100);
client.setSampleMode(SAMPLE_INTERPOLATED);  // Smooth scaling
```

---

### Transform Configuration

#### `void setSerpentine(uint8_t mode)`

Set serpentine wiring mode for LED matrices.

**Parameters:**
- `mode` - `SERPENTINE_NONE`, `SERPENTINE_HORIZONTAL`, or `SERPENTINE_VERTICAL`

**Modes:**

**`SERPENTINE_NONE`** - Linear wiring:
```
0 → 1 → 2 → 3
4 → 5 → 6 → 7
8 → 9 → 10 → 11
```

**`SERPENTINE_HORIZONTAL`** - Zigzag left-right:
```
0  → 1  → 2  → 3
7  ← 6  ← 5  ← 4
8  → 9  → 10 → 11
```

**`SERPENTINE_VERTICAL`** - Zigzag up-down:
```
0   8   16
↓   ↑   ↓
1   9   17
↓   ↑   ↓
2   10  18
```

**Example:**
```cpp
client.setRectangle(0, 0, 16, 16);
client.setSerpentine(SERPENTINE_HORIZONTAL);
```

---

#### `void setRotation(uint8_t rotation)`

Rotate the image before mapping to LEDs.

**Parameters:**
- `rotation` - `0`, `1`, `2`, or `3`

**Rotation values:**
- `0` - No rotation (0°)
- `1` - 90° clockwise
- `2` - 180°
- `3` - 270° clockwise (90° counter-clockwise)

**Use case:** Physically mounted display at wrong angle

**Example:**
```cpp
client.setRotation(2);  // Display mounted upside-down
```

---

#### `void setFlip(bool flipX, bool flipY, bool flipZ)`

Mirror the image along different axes.

**Parameters:**
- `flipX` - Horizontal flip (left ↔ right)
- `flipY` - Vertical flip (top ↔ bottom)
- `flipZ` - Diagonal flip (transpose)

**Flip combinations:**
- `flipX=true` - Mirror horizontally
- `flipY=true` - Mirror vertically
- `flipX=true, flipY=true` - Same as 180° rotation
- `flipZ=true` - Transpose (swap width ↔ height)

**Example:**
```cpp
// Mirror horizontally only
client.setFlip(true, false, false);

// Reset all flips
client.setFlip(false, false, false);
```

---

### Display Control

#### `void setBrightness(uint8_t brightness)`

Set global LED brightness.

**Parameters:**
- `brightness` - 0 (off) to 255 (full brightness)

**Note:** Applied immediately to all LEDs

**Example:**
```cpp
client.setBrightness(80);   // ~30% brightness
client.setBrightness(255);  // Full brightness
```

---

#### `void clear(bool show = true)`

Turn off all LEDs.

**Parameters:**
- `show` - If `true`, immediately update LEDs; if `false`, buffer only

**Example:**
```cpp
client.clear();  // Turn off all LEDs immediately
```

---

### Status and Statistics

#### `const FataMorganaMapping& getMapping() const`

Get current mapping configuration.

**Returns:** Reference to `FataMorganaMapping` struct

**Example:**
```cpp
const FataMorganaMapping& mapping = client.getMapping();
Serial.printf("Mode: %s\n", fatamorgana_mappingModeName(mapping.mode));
Serial.printf("Rectangle: %ux%u at (%u,%u)\n",
              mapping.rectWidth, mapping.rectHeight,
              mapping.rectX, mapping.rectY);
```

---

#### `uint32_t getRenderedFrames() const`

Get count of successfully rendered frames since startup.

**Returns:** Number of complete frames displayed

**Example:**
```cpp
Serial.printf("Rendered %lu frames\n", client.getRenderedFrames());
```

---

#### `uint32_t getAcceptedPackets() const`

Get count of accepted UDP packets.

**Returns:** Number of valid packets processed

**Example:**
```cpp
Serial.printf("Packets: %lu accepted\n", client.getAcceptedPackets());
```

---

#### `uint32_t getRejectedPackets() const`

Get count of rejected UDP packets.

**Returns:** Number of invalid/duplicate packets discarded

**Rejection reasons:**
- Wrong frame ID
- Out-of-sequence packet
- Invalid packet size
- Corrupted data

**Example:**
```cpp
uint32_t accepted = client.getAcceptedPackets();
uint32_t rejected = client.getRejectedPackets();
float loss = 100.0f * rejected / (accepted + rejected);
Serial.printf("Packet loss: %.2f%%\n", loss);
```

---

## FataMorganaWebUI

Optional web-based configuration interface.

**Requires:** `FataMorgana-WebUI` library

### Constructor

```cpp
FataMorganaWebUI(FataMorganaClient& client)
```

**Parameters:**
- `client` - Reference to your `FataMorganaClient` instance

**Example:**
```cpp
FataMorganaClient client(256, 12);
FataMorganaWebUI webUI(client);
```

---

### Lifecycle Methods

#### `bool begin(uint16_t httpPort = 80, uint16_t wsPort = 81)`

Start HTTP and WebSocket servers.

**Parameters:**
- `httpPort` - HTTP server port (default: 80)
- `wsPort` - WebSocket server port (default: 81)

**Returns:** `true` on success, `false` on failure

**Example:**
```cpp
void setup() {
  client.begin();
  webUI.begin();  // Default ports 80, 81

  // Or custom ports:
  // webUI.begin(8080, 8081);
}
```

---

#### `void loop()`

Handle HTTP requests and WebSocket connections.

**Must be called:** Continuously in your main loop

**Example:**
```cpp
void loop() {
  client.loop();
  webUI.loop();
}
```

---

### Configuration

#### `void enableWebSocket(bool enable = true)`

Enable/disable WebSocket real-time updates.

**Parameters:**
- `enable` - `true` to enable WebSocket, `false` to disable

**Default:** Enabled

**Example:**
```cpp
webUI.enableWebSocket(false);  // Disable WebSocket to save memory
```

---

#### `void broadcastStatus()`

Send current status to all WebSocket clients.

**Use case:** Manual status updates after configuration changes

**Example:**
```cpp
client.setRectangle(8, 8, 16, 16);
webUI.broadcastStatus();  // Notify web clients
```

---

## FataMorganaMapping

Configuration structure returned by `getMapping()`.

### Structure

```cpp
struct FataMorganaMapping {
  uint8_t mode;          // MAPPING_ROW, MAPPING_COLUMN, or MAPPING_RECTANGLE
  uint8_t sampleMode;    // SAMPLE_PIXEL or SAMPLE_INTERPOLATED

  // Rectangle mode
  uint16_t rectX;        // Left edge
  uint16_t rectY;        // Top edge
  uint16_t rectWidth;    // Width in pixels
  uint16_t rectHeight;   // Height in pixels

  // Row/Column mode
  uint16_t rowIndex;     // Row Y coordinate
  uint16_t columnIndex;  // Column X coordinate
  uint16_t linePixels;   // Number of pixels to sample

  // Transform settings
  uint8_t serpentine;    // SERPENTINE_NONE, SERPENTINE_HORIZONTAL, or SERPENTINE_VERTICAL
  uint8_t rotation;      // 0, 1, 2, or 3 (0°, 90°, 180°, 270°)
  bool flipX;            // Horizontal flip
  bool flipY;            // Vertical flip
  bool flipZ;            // Diagonal flip (transpose)
};
```

### Example Usage

```cpp
void printConfig() {
  const FataMorganaMapping& m = client.getMapping();

  if (m.mode == MAPPING_RECTANGLE) {
    Serial.printf("Rectangle: %ux%u at (%u,%u)\n",
                  m.rectWidth, m.rectHeight, m.rectX, m.rectY);
  } else if (m.mode == MAPPING_ROW) {
    Serial.printf("Row %u, %u pixels\n", m.rowIndex, m.linePixels);
  } else {
    Serial.printf("Column %u, %u pixels\n", m.columnIndex, m.linePixels);
  }

  Serial.printf("Rotation: %u°\n", m.rotation * 90);
  Serial.printf("Flips: X=%d Y=%d Z=%d\n", m.flipX, m.flipY, m.flipZ);
  Serial.printf("Serpentine: %s\n", fatamorgana_serpentineModeName(m.serpentine));
}
```

---

## Protocol Constants

Defined in `FataMorganaProtocol.h`.

### Network

```cpp
const IPAddress FATAMORGANA_MULTICAST_ADDR(239, 255, 42, 1);
constexpr uint16_t FATAMORGANA_MULTICAST_PORT = 7777;  // Server → Devices
constexpr uint16_t FATAMORGANA_RESPONSE_PORT = 7778;    // Devices → Server
```

### Packet Limits

```cpp
constexpr size_t FATAMORGANA_HEADER_SIZE = 8;
constexpr size_t FATAMORGANA_PACKET_DATA_SIZE = 1400;
constexpr size_t FATAMORGANA_MAX_FRAME_BYTES = 4800;
```

### RGB Encoding

```cpp
constexpr uint8_t FATAMORGANA_RGB332 = 0;  // 1 byte per pixel
constexpr uint8_t FATAMORGANA_RGB565 = 1;  // 2 bytes per pixel
```

### Mapping Modes

```cpp
constexpr uint8_t MAPPING_ROW = 0;
constexpr uint8_t MAPPING_COLUMN = 1;
constexpr uint8_t MAPPING_RECTANGLE = 2;
```

### Sampling Modes

```cpp
constexpr uint8_t SAMPLE_PIXEL = 0;         // Nearest-neighbor
constexpr uint8_t SAMPLE_INTERPOLATED = 1;  // Bilinear
```

### Serpentine Modes

```cpp
constexpr uint8_t SERPENTINE_NONE = 0;
constexpr uint8_t SERPENTINE_HORIZONTAL = 1;
constexpr uint8_t SERPENTINE_VERTICAL = 2;
```

---

## Helper Functions

Utility functions for working with the protocol.

### `fatamorgana_frameSize()`

Calculate frame size in bytes.

```cpp
size_t fatamorgana_frameSize(uint16_t width, uint16_t height, uint8_t rgbType);
```

**Parameters:**
- `width` - Frame width in pixels
- `height` - Frame height in pixels
- `rgbType` - `FATAMORGANA_RGB332` or `FATAMORGANA_RGB565`

**Returns:** Frame size in bytes (including header)

**Example:**
```cpp
size_t size = fatamorgana_frameSize(24, 100, FATAMORGANA_RGB332);
// Returns: 2408 bytes (8 header + 2400 data)
```

---

### `fatamorgana_mappingModeName()`

Get human-readable mapping mode name.

```cpp
const char* fatamorgana_mappingModeName(uint8_t mode);
```

**Parameters:**
- `mode` - `MAPPING_ROW`, `MAPPING_COLUMN`, or `MAPPING_RECTANGLE`

**Returns:** String name ("Row", "Column", or "Rectangle")

**Example:**
```cpp
Serial.println(fatamorgana_mappingModeName(MAPPING_RECTANGLE));
// Prints: "Rectangle"
```

---

### `fatamorgana_sampleModeName()`

Get human-readable sample mode name.

```cpp
const char* fatamorgana_sampleModeName(uint8_t mode);
```

**Parameters:**
- `mode` - `SAMPLE_PIXEL` or `SAMPLE_INTERPOLATED`

**Returns:** String name ("Pixel" or "Interpolated")

**Example:**
```cpp
Serial.println(fatamorgana_sampleModeName(SAMPLE_INTERPOLATED));
// Prints: "Interpolated"
```

---

### `fatamorgana_serpentineModeName()`

Get human-readable serpentine mode name.

```cpp
const char* fatamorgana_serpentineModeName(uint8_t mode);
```

**Parameters:**
- `mode` - `SERPENTINE_NONE`, `SERPENTINE_HORIZONTAL`, or `SERPENTINE_VERTICAL`

**Returns:** String name ("None", "Horizontal", or "Vertical")

**Example:**
```cpp
Serial.println(fatamorgana_serpentineModeName(SERPENTINE_HORIZONTAL));
// Prints: "Horizontal"
```

---

## Complete Example

Putting it all together:

```cpp
#include <ESP8266WiFi.h>
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>

const char* WIFI_SSID = "YourNetwork";
const char* WIFI_PASSWORD = "YourPassword";

FataMorganaClient client(256, 12);  // 256 LEDs on GPIO 12
FataMorganaWebUI webUI(client);

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Initialize FataMorgana
  if (!client.begin()) {
    Serial.println("Failed to initialize!");
    return;
  }

  // Configure mapping (16x16 matrix)
  client.setRectangle(0, 0, 16, 16);
  client.setSerpentine(SERPENTINE_HORIZONTAL);
  client.setRotation(0);
  client.setBrightness(80);

  // Start web interface
  webUI.begin();

  Serial.print("Web UI: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  client.loop();
  webUI.loop();

  // Print statistics every 10 seconds
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 10000) {
    lastStats = millis();

    Serial.printf("Frames: %lu, Packets: %lu/%lu\n",
                  client.getRenderedFrames(),
                  client.getAcceptedPackets(),
                  client.getRejectedPackets());

    const FataMorganaMapping& m = client.getMapping();
    Serial.printf("Mode: %s, Rotation: %u°\n",
                  fatamorgana_mappingModeName(m.mode),
                  m.rotation * 90);
  }
}
```

---

## Performance Tips

### Memory Usage

- **WebUI library** adds ~15KB flash, ~2KB RAM
- **RGB332** uses half the frame buffer of RGB565
- Disable WebSocket for slight memory savings: `webUI.enableWebSocket(false)`

### Speed

- **SAMPLE_PIXEL** is faster than SAMPLE_INTERPOLATED
- **SERPENTINE_NONE** is fastest (no coordinate transforms)
- Rectangle mode is faster than row/column (direct memory copy)

### Network

- Keep frame size under 4800 bytes to avoid fragmentation
- Use RGB332 for large displays (24×100 = 2400 bytes vs 4800)
- Multicast works best on same subnet

---

## See Also

- [GETTING_STARTED.md](GETTING_STARTED.md) - Setup guide
- [TRANSFORM_FEATURE.md](TRANSFORM_FEATURE.md) - Transform details
- [FATAMORGANA_PROTOCOL.md](FATAMORGANA_PROTOCOL.md) - Protocol specification
- [LIBRARY_ARCHITECTURE.md](LIBRARY_ARCHITECTURE.md) - Design decisions
