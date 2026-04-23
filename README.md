# FataMorgana

**High-performance distributed LED display system for ESP8266/ESP32**

FataMorgana is a UDP multicast protocol and library for controlling multiple LED strips/matrices from a central server. Each ESP device extracts its portion of the image and displays it on connected LEDs, enabling large-scale synchronized LED installations.

---

## Features

### Protocol
- **UDP Multicast** - Efficient broadcast to all devices (239.255.42.1:7777)
- **Binary Format** - Compact 8-byte headers, RGB332/RGB565 encoding
- **Chunked Frames** - Support for large images up to 4800 bytes
- **Auto-Discovery** - Devices announce themselves to the server
- **Device Identification** - Flash LEDs on specific devices to locate them physically

### Mapping Modes
- **Rectangle** - Extract rectangular region from image
- **Row/Column** - Extract single line with sampling
- **Interpolation** - Smooth scaling with bilinear sampling

### Transforms
- **Rotation** - 0°, 90°, 180°, 270° rotation support
- **Flips** - Horizontal, vertical, diagonal mirroring
- **Serpentine** - Zigzag LED matrix wiring (horizontal/vertical)
- **Out-of-Bounds** - Black, clamp (edge repeat), or mirror modes

### Hardware Support
- ESP8266 and ESP32 platforms
- WS2812/WS2812B/NeoPixel LED strips
- Any NeoPixel-compatible RGB(W) LED

---

## Quick Start

### Hardware Setup

```
ESP8266/ESP32          LED Strip
--------------        -----------
GPIO 12 ---------->   DIN (Data)
GND   -------------->  GND
5V (from PSU) ------>  5V
```

**Important:** Use external 5V power supply for LEDs!

### Install Libraries

#### PlatformIO

```ini
[env:d1_mini]
platform = espressif8266
board = d1_mini
framework = arduino

lib_deps =
    FataMorgana           # Core protocol
    FataMorgana-WebUI     # Optional web interface
```

#### Arduino IDE

1. **Sketch → Include Library → Manage Libraries**
2. Search for "FataMorgana"
3. Install **FataMorgana** (required)
4. Install **FataMorgana-WebUI** (optional)

### Minimal Example

```cpp
#include <ESP8266WiFi.h>
#include <FataMorgana.h>

const char* WIFI_SSID = "YourNetwork";
const char* WIFI_PASSWORD = "YourPassword";

FataMorganaClient client(256, 12);  // 256 LEDs on GPIO 12

void setup() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.begin();
  client.setRectangle(0, 0, 16, 16);  // Extract 16x16 region
}

void loop() {
  client.loop();
}
```

### With Web Interface

```cpp
#include <ESP8266WiFi.h>
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>

FataMorganaClient client(256, 12);
FataMorganaWebUI webUI(client);

void setup() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.begin();
  webUI.begin();  // Start web server on port 80

  client.setRectangle(0, 0, 16, 16);
  client.setSerpentine(SERPENTINE_HORIZONTAL);
}

void loop() {
  client.loop();
  webUI.loop();
}
```

Open `http://<device-ip>` to configure via web interface.

---

## Server Setup

The Node.js server sends image frames to all devices:

```bash
cd server
npm install
npm start
```

Access server interface: `http://localhost:3001`

### Server Features

The web control panel provides:
- **Pattern Generator** - Send test patterns (gradient, solid, checkerboard, rainbow)
- **Image Upload** - Upload and broadcast custom images
- **Rainbow Animation** - Continuous animated rainbow (1-80 FPS, adjustable resolution/encoding)
- **Device Discovery** - Find all devices on the network
- **Device Identification** - Flash specific devices to locate them
- **Live Visualizer** - See frames with device regions overlaid
- **Coverage Map** - Calculate pixel coverage across devices

### Server API Examples

**Discover devices:**
```bash
curl -X POST http://localhost:3001/api/discover
```

**Send test frame:**
```bash
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{"width":24,"height":100,"pattern":"gradient"}'
```

**Identify device (flash LEDs):**
```bash
curl -X POST http://localhost:3001/api/identify \
  -H "Content-Type: application/json" \
  -d '{"ip":"192.168.1.100","flashCount":5}'
```

**Upload custom image:**
```bash
curl -X POST http://localhost:3001/api/frame/image \
  -H "Content-Type: application/json" \
  -d '{"image":"data:image/png;base64,...","targetWidth":24,"targetHeight":100}'
```

---

## Architecture

FataMorgana uses a **two-library architecture** for flexibility:

### FataMorgana (Core)
- UDP multicast protocol implementation
- Frame reception and rendering
- Mapping modes and transforms
- **Minimal dependencies** (only Adafruit_NeoPixel)
- Required for all projects

### FataMorgana-WebUI (Optional)
- Web-based configuration interface
- HTTP REST API
- WebSocket real-time updates
- **Optional** - use only if you need web config

This separation allows you to:
- Use core protocol without web overhead
- Create custom configuration interfaces (MQTT, BLE, Serial, etc.)
- Keep dependencies minimal for production deployments

Read more: [LIBRARY_ARCHITECTURE.md](LIBRARY_ARCHITECTURE.md)

---

## Examples

All examples included with libraries:

### FataMorgana Core
- **BasicClient** - Minimal example (50 lines)
- **CustomMapping** - Row/Column/Rectangle modes
- **RectangleWithTransforms** - Rotation and flips

### FataMorgana-WebUI
- **WithWebInterface** - Full web configuration

Open examples in Arduino IDE:
**File → Examples → FataMorgana → BasicClient**

---

## Documentation

- **[Getting Started](GETTING_STARTED.md)** - Complete setup guide
- **[API Reference](API.md)** - Complete API documentation
- **[Protocol Specification](FATAMORGANA_PROTOCOL.md)** - Binary protocol details
- **[Library Architecture](LIBRARY_ARCHITECTURE.md)** - Design decisions
- **[Transform Feature](TRANSFORM_FEATURE.md)** - Rotation and flip guide
- **[Network Configuration](NETWORK_CONFIG.md)** - Multicast setup
- **[Changelog](CHANGELOG.md)** - Version history

---

## Use Cases

### Large LED Walls
Split a large image across multiple ESP controllers, each driving a section of the display.

```
Image: 48x48 pixels
├── Device 1: Extract (0,0) 16x48
├── Device 2: Extract (16,0) 16x48
└── Device 3: Extract (32,0) 16x48
```

### LED Strip Arrays
Display different rows/columns on separate LED strips.

```
Image: 24x100 pixels
├── Strip 1: Row 0
├── Strip 2: Row 1
└── Strip 3: Row 2
```

### Synchronized Effects
All devices receive the same frame simultaneously via multicast, ensuring perfect synchronization.

---

## Performance

| Metric | Value |
|--------|-------|
| Max frame size | 4800 bytes |
| RGB332 max resolution | 69×69 pixels |
| RGB565 max resolution | 48×48 pixels |
| Theoretical FPS | 30-60 fps |
| Network bandwidth (48×48 @ 30fps) | ~1.15 Mbps |
| Inter-packet delay | 2-5ms (recommended) |

---

## Configuration

### Via Server Control Panel

Access `http://localhost:3001` for centralized control:
- **Discover** all devices on network
- **Identify** specific devices (flash LEDs to locate physically)
- **Send** test patterns or custom images
- **Visualize** frame data with device regions
- **Calculate** coverage maps

### Via Device Web Interface

Access `http://<device-ip>` to configure individual devices:
- Mapping mode (Rectangle/Row/Column)
- Position and size
- Rotation and flips
- Serpentine wiring mode
- Out-of-bounds mode (Black/Clamp/Mirror)
- Gamma correction
- Brightness

Changes auto-save and re-render immediately.

### Via Serial Commands

Implement your own serial command parser:

```cpp
if (Serial.available()) {
  String cmd = Serial.readStringUntil('\n');
  if (cmd.startsWith("rect ")) {
    int x, y, w, h;
    sscanf(cmd.c_str(), "rect %d %d %d %d", &x, &y, &w, &h);
    client.setRectangle(x, y, w, h);
  }
}
```

### Via Code

All configuration methods available in API:

```cpp
// Rectangle mode
client.setRectangle(0, 0, 16, 16);
client.setSerpentine(SERPENTINE_HORIZONTAL);

// Row mode with interpolation
client.setRowMapping(5, 100);
client.setSampleMode(SAMPLE_INTERPOLATED);

// Transforms
client.setRotation(2);  // 180° rotation
client.setFlip(true, false, false);  // Horizontal flip
client.setOOBMode(FATAMORGANA_OOB_CLAMP);  // Clamp out-of-bounds pixels
client.setBrightness(80);
```

---

## Troubleshooting

### LEDs Not Lighting Up

**Check:**
- ✅ ESP connected to WiFi (Serial Monitor)
- ✅ Multicast group joined successfully
- ✅ Server sending frames (server logs)
- ✅ Firewall allows UDP 7777
- ✅ Router supports multicast/IGMP
- ✅ LED power supply adequate
- ✅ LED data pin correct (GPIO 12)

**Debug:**
```cpp
void loop() {
  client.loop();

  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("Frames: %lu, Packets: %lu/%lu\n",
                  client.getRenderedFrames(),
                  client.getAcceptedPackets(),
                  client.getRejectedPackets());
  }
}
```

### Multicast Not Working

**Windows:**
```bash
route print  # Check for 239.0.0.0 route
```

**Linux:**
```bash
sudo ip route add 239.0.0.0/8 dev eth0
```

**Router:**
- Enable IGMP Snooping
- Allow multicast forwarding

### Wrong Colors/Pattern

**Check:**
- LED type matches code: `NEO_GRB` vs `NEO_RGB`
- Serpentine mode matches physical wiring
- Rectangle position/size correct
- No accidental rotation/flips

See [GETTING_STARTED.md](GETTING_STARTED.md) for complete troubleshooting guide.

---

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with tests
4. Update documentation
5. Commit (`git commit -m 'Add amazing feature'`)
6. Push (`git push origin feature/amazing-feature`)
7. Open a Pull Request

### Development Setup

```bash
# Clone repository
git clone https://github.com/yourname/FataMorgana.git
cd FataMorgana

# Install PlatformIO
pip install platformio

# Build
pio run

# Upload to device
pio run -t upload

# Monitor serial output
pio device monitor
```

---

## Future Extensions

### Planned
- Configuration persistence (EEPROM/Flash)
- OTA firmware updates
- Frame acknowledgment and retransmission
- Compression (RLE, delta encoding)

### Community Extensions
- **FataMorgana-MQTT** - MQTT control interface
- **FataMorgana-BLE** - Bluetooth configuration
- **FataMorgana-HomeAssistant** - Home Assistant integration
- **FataMorgana-ESPNow** - ESP-NOW transport

Build your own extension! The modular architecture makes it easy.

---

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Authors

- [Your Name] - Initial work and protocol design

## Acknowledgments

- Adafruit for the NeoPixel library
- ESP8266/ESP32 community for platform support
- All contributors and testers

---

## Support

**Issues:** [GitHub Issues](https://github.com/yourname/FataMorgana/issues)

**Questions:** Open a [Discussion](https://github.com/yourname/FataMorgana/discussions)

**Show your build:** Tag `#FataMorgana` and share your creation!

---

## Project Status

**Version:** 1.0.0
**Status:** Production Ready
**Platforms:** ESP8266, ESP32
**License:** MIT

Happy building! ✨
