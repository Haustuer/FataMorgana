# FataMorgana LED Display Library

A high-performance UDP multicast protocol library for distributed LED display systems.

> **Core Protocol Library** - This library contains only the FataMorgana protocol implementation.
> For optional web configuration interface, see **FataMorgana-WebUI** (separate library).

## Features

- **UDP Multicast Protocol** - Efficient one-to-many frame distribution
- **Multiple Mapping Modes** - Row, Column, and Rectangle extraction
- **Advanced Transforms** - Rotation (0°/90°/180°/270°), Flip X/Y/Z
- **Serpentine Layouts** - Support for horizontal and vertical LED wiring patterns
- **Dual RGB Encoding** - RGB332 (1 byte/pixel) and RGB565 (2 bytes/pixel)
- **Optional Web Interface** - Real-time configuration with WebSocket updates
- **Auto-Discovery** - Automatic device discovery and registration
- **High Performance** - Optimized for ESP8266/ESP32
- **Interpolated Sampling** - Smooth scaling for row/column modes

## Quick Start

### Installation

#### PlatformIO
```ini
lib_deps =
    FataMorgana
```

#### Arduino IDE
1. Download the library
2. Sketch → Include Library → Add .ZIP Library
3. Select the downloaded file

### Basic Example

```cpp
#include <FataMorgana.h>

FataMorganaClient client(256, 12);  // 256 LEDs on pin 12

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Initialize FataMorgana
  client.begin();

  // Configure rectangle mapping
  client.setRectangle(0, 0, 16, 16);
  client.setSerpentine(SERPENTINE_HORIZONTAL);
}

void loop() {
  client.loop();
}
```

### With Web Interface (Optional)

For web-based configuration, install the separate **FataMorgana-WebUI** library:

```ini
lib_deps =
    FataMorgana
    FataMorgana-WebUI
```

```cpp
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>  // Separate library

FataMorganaClient client(256, 12);
FataMorganaWebUI webUI(client);

void setup() {
  Serial.begin(115200);
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");

  client.begin();
  webUI.begin();  // HTTP on port 80, WebSocket on port 81

  Serial.print("Web interface: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  client.loop();
  webUI.loop();
}
```

See **FataMorgana-WebUI** library for details.

## API Reference

### FataMorganaClient

#### Constructor
```cpp
FataMorganaClient(uint16_t ledCount, uint8_t ledPin, uint8_t ledType = NEO_GRB + NEO_KHZ800)
```

#### Methods
- `void begin()` - Initialize the client
- `void loop()` - Process incoming frames (call in main loop)
- `void setRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height)` - Set rectangle mapping
- `void setRowMapping(uint16_t rowIndex, uint16_t pixels)` - Set row mapping
- `void setColumnMapping(uint16_t columnIndex, uint16_t pixels)` - Set column mapping
- `void setSerpentine(uint8_t mode)` - Set serpentine mode (NONE/HORIZONTAL/VERTICAL)
- `void setRotation(uint8_t rotation)` - Set rotation (0-3 for 0°/90°/180°/270°)
- `void setFlip(bool x, bool y, bool z)` - Set flip transforms
- `uint32_t getRenderedFrames()` - Get rendered frame count
- `uint32_t getAcceptedPackets()` - Get accepted packet count
- `uint32_t getRejectedPackets()` - Get rejected packet count

### FataMorganaWebUI

#### Constructor
```cpp
FataMorganaWebUI(FataMorganaClient& client)
```

#### Methods
- `void begin(uint16_t httpPort = 80, uint16_t wsPort = 81)` - Start web server
- `void loop()` - Handle web requests (call in main loop)

## Configuration

### Mapping Modes

**Rectangle Mode** - Extract a rectangular region
```cpp
client.setRectangle(0, 0, 16, 16);  // Top-left 16×16
```

**Row Mode** - Extract a horizontal line
```cpp
client.setRowMapping(10, 100);  // Row 10, 100 LEDs
```

**Column Mode** - Extract a vertical line
```cpp
client.setColumnMapping(5, 50);  // Column 5, 50 LEDs
```

### Serpentine Layouts

For LED strips wired in zigzag patterns:

```cpp
client.setSerpentine(SERPENTINE_HORIZONTAL);  // Zigzag left-right
client.setSerpentine(SERPENTINE_VERTICAL);    // Zigzag up-down
client.setSerpentine(SERPENTINE_NONE);        // Normal layout
```

### Transforms

```cpp
// Rotation
client.setRotation(1);  // 90° clockwise
client.setRotation(2);  // 180°
client.setRotation(3);  // 270° clockwise

// Flips
client.setFlip(true, false, false);  // Flip X (horizontal mirror)
client.setFlip(false, true, false);  // Flip Y (vertical mirror)
client.setFlip(false, false, true);  // Flip Z (transpose)
```

## Network Configuration

### Multicast Settings
- **Multicast Group**: 239.255.42.1
- **Multicast Port**: 7777 (server → devices)
- **Response Port**: 7778 (devices → server)

### Firewall Rules
Ensure UDP ports 7777 and 7778 are open on your network.

## Hardware Requirements

- ESP8266 or ESP32
- NeoPixel compatible LED strip (WS2812B, WS2811, etc.)
- WiFi connection

## Examples

See the `examples/` directory for:
- **BasicClient** - Minimal setup
- **WithWebInterface** - Full web UI
- **CustomMapping** - Different mapping modes
- **RectangleWithTransforms** - Rotation and flips

## Protocol Documentation

See `PROTOCOL.md` for complete protocol specification.

## License

MIT License - See LICENSE file

## Contributing

Contributions welcome! Please submit pull requests or open issues on GitHub.

## Support

For bugs and feature requests, please open an issue on GitHub.
