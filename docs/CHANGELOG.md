# Changelog

All notable changes to the FataMorgana LED display library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial library release with two-library architecture
- FataMorgana core protocol library
- FataMorgana-WebUI optional web interface library

## [1.0.0] - 2026-04-22

### Added

#### Core Protocol (FataMorgana)
- UDP multicast protocol (239.255.42.1:7777) for server-to-device communication
- Unicast response channel (port 7778) for device-to-server communication
- Binary packet format with 8-byte headers
- RGB332 and RGB565 pixel encoding support
- Chunked frame transmission for large images (up to 4800 bytes)
- Frame reassembly with duplicate detection and validation

#### Mapping Modes
- **Rectangle mode** - Extract rectangular region from image
- **Row mode** - Extract horizontal line with configurable sampling
- **Column mode** - Extract vertical line with configurable sampling
- Sample modes: Pixel (nearest-neighbor) and Interpolated (bilinear)

#### Transform System
- Rotation support (0°, 90°, 180°, 270°)
- Flip operations (X: horizontal, Y: vertical, Z: diagonal/transpose)
- Serpentine layout support (horizontal zigzag, vertical zigzag)
- Transform pipeline applied before LED rendering

#### Discovery Protocol
- Binary discovery request (Type 0, SubType 0)
- 64-byte binary discovery response format
- Magic bytes (0x46415441 "FATA") for validation
- Device configuration and statistics reporting
- Network-byte-order for network fields, little-endian for device data

#### Client API
- `FataMorganaClient` class for ESP8266/ESP32
- Configuration methods: `setRectangle()`, `setRowMapping()`, `setColumnMapping()`
- Transform methods: `setRotation()`, `setFlip()`, `setSerpentine()`
- Display control: `setBrightness()`, `clear()`
- Statistics: `getRenderedFrames()`, `getAcceptedPackets()`, `getRejectedPackets()`
- Mapping inspection: `getMapping()`

#### Web Interface (FataMorgana-WebUI)
- Optional web-based configuration interface (separate library)
- HTTP server with REST API endpoints
- WebSocket real-time status updates
- Auto-save configuration (300ms debounce)
- Responsive single-page interface
- Configuration persistence in device memory
- Automatic re-render on configuration change

#### Examples
- **BasicClient** - Minimal 50-line example (no web interface)
- **CustomMapping** - Interactive mapping mode demonstration
- **RectangleWithTransforms** - Transform configuration via Serial
- **WithWebInterface** - Full example with web UI and statistics

#### Documentation
- Complete API reference (API.md)
- Getting started guide with hardware setup (GETTING_STARTED.md)
- Protocol specification (FATAMORGANA_PROTOCOL.md)
- Library architecture explanation (LIBRARY_ARCHITECTURE.md)
- Transform feature guide (TRANSFORM_FEATURE.md)
- Network configuration guide (NETWORK_CONFIG.md)

#### Build System
- PlatformIO library.json for both libraries
- Arduino library.properties for both libraries
- Keywords.txt for syntax highlighting
- Local library support in platformio.ini
- Example projects included in libraries

### Changed
- Refactored monolithic main.cpp (1000+ lines) into reusable library architecture
- Separated core protocol from web interface into two distinct libraries
- Improved modularity for community extensions (MQTT, BLE, etc.)

### Technical Details

#### Dependencies
**FataMorgana (core):**
- Adafruit_NeoPixel ^1.12.3

**FataMorgana-WebUI:**
- FataMorgana ^1.0.0
- ArduinoJson ^7.0.4
- WebSockets (links2004) ^2.4.1

#### Platforms
- ESP8266 (espressif8266)
- ESP32 (espressif32)

#### Frameworks
- Arduino
- ESP-IDF

### Performance
- Frame buffer: up to 4800 bytes
- Theoretical frame rate: 30-60 fps (ESP processing dependent)
- Network bandwidth: ~1.15 Mbps for 48x48 RGB565 @ 30fps
- Recommended inter-packet delay: 2-5ms

### Known Limitations
- No packet retransmission (UDP reliability)
- No frame acknowledgment
- No encryption or authentication
- Maximum frame size: 4800 bytes
- RGB332: max 69x69 pixels
- RGB565: max 48x48 pixels

### Security
- **WARNING**: No authentication or encryption in v1.0.0
- Only use on trusted networks
- Not suitable for public networks without additional security measures

---

## Future Roadmap

### Planned for v1.1.0
- Configuration persistence to EEPROM/Flash
- Over-the-air (OTA) firmware updates
- Extended statistics and diagnostics

### Planned for v2.0.0
- Remote configuration via protocol (SET_MAPPING, SET_BRIGHTNESS)
- Frame acknowledgment and retransmission
- Delta encoding for animation optimization
- RLE compression for solid regions
- Timestamp-based frame synchronization

### Potential Extensions
- FataMorgana-MQTT - MQTT configuration interface
- FataMorgana-BLE - Bluetooth Low Energy control
- FataMorgana-HomeAssistant - Home Assistant integration
- FataMorgana-ESPNow - ESP-NOW alternative transport

---

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Update documentation
5. Submit a pull request

## License

MIT License - See LICENSE file for details

## Authors

- [Your Name] - Initial work and protocol design

## Acknowledgments

- Adafruit for NeoPixel library
- ESP8266/ESP32 community
- All contributors and testers
