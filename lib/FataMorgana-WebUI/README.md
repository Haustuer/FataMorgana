# FataMorgana-WebUI

Optional web configuration interface for FataMorgana LED display library.

## Overview

FataMorgana-WebUI is an **optional helper library** that provides a web-based configuration interface for FataMorgana devices. It is **NOT part of the core FataMorgana protocol** - it's a convenience tool for users who want remote configuration capabilities.

## Features

- **HTTP Web Interface** - Browser-based configuration page
- **WebSocket Real-Time Updates** - Live status updates (5 times/second)
- **Auto-Save Configuration** - Changes apply automatically as you adjust settings
- **Responsive UI** - Works on desktop and mobile devices
- **Zero Configuration** - Works out-of-the-box with sensible defaults

## When to Use This

Use FataMorgana-WebUI if you want:
- ✅ Remote configuration via web browser
- ✅ Real-time status monitoring
- ✅ Easy-to-use UI for non-technical users
- ✅ No need to recompile/upload firmware for config changes

Skip FataMorgana-WebUI if you:
- ❌ Prefer other configuration methods (MQTT, Serial, REST API, etc.)
- ❌ Want minimal memory footprint
- ❌ Are using non-ESP platforms
- ❌ Have fixed configuration (no runtime changes needed)

## Installation

### PlatformIO
```ini
lib_deps =
    FataMorgana
    FataMorgana-WebUI
```

### Arduino IDE
1. Install FataMorgana library first
2. Install FataMorgana-WebUI library

## Quick Start

```cpp
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>

FataMorganaClient client(256, 12);  // 256 LEDs on pin 12
FataMorganaWebUI webUI(client);     // Attach web interface

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Initialize FataMorgana client
  client.begin();
  client.setRectangle(0, 0, 16, 16);

  // Start web interface
  webUI.begin();  // HTTP on port 80, WebSocket on port 81

  Serial.print("Web interface: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  client.loop();  // Process UDP frames
  webUI.loop();   // Process web requests
}
```

## API Reference

### FataMorganaWebUI

#### Constructor
```cpp
FataMorganaWebUI(FataMorganaClient& client)
```

#### Methods

**`bool begin(uint16_t httpPort = 80, uint16_t wsPort = 81)`**
- Start HTTP and WebSocket servers
- Returns `true` if successful

**`void loop()`**
- Process web requests and WebSocket messages
- Call this in your main loop

**`void enableWebSocket(bool enable = true)`**
- Enable/disable WebSocket broadcasting
- Useful for temporarily pausing updates

**`void broadcastStatus()`**
- Manually broadcast status to all WebSocket clients
- Called automatically every 200ms

## Web Interface

Access the web interface by navigating to:
```
http://<device-ip>/
```

### Features

**Configuration Panel:**
- Mapping mode selection (Row/Column/Rectangle)
- Rectangle position and size
- Serpentine mode (None/Horizontal/Vertical)
- Rotation (0°/90°/180°/270°)
- Flip transforms (X/Y/Z)
- Auto-save on change (300ms debounce)

**Status Panel:**
- Device IP and WiFi status
- WebSocket connection status
- LED count and mapping summary
- Frame statistics
- Packet counters
- Real-time updates

## Network Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 80 | HTTP | Web interface (configurable) |
| 81 | WebSocket | Real-time status updates (configurable) |

## Memory Usage

Approximate memory footprint:
- **Flash**: ~15KB (HTML + code)
- **RAM**: ~4KB (buffers + WebSocket)

## Customization

### Custom Ports
```cpp
webUI.begin(8080, 8081);  // HTTP on 8080, WS on 8081
```

### Disable WebSocket
```cpp
webUI.enableWebSocket(false);  // Use HTTP polling only
```

## Security Considerations

⚠️ **Important**: This library provides NO authentication or encryption.

**Recommendations:**
- Only use on trusted networks
- Consider adding authentication wrapper
- Use firewall rules to restrict access
- For production, implement proper security layer

## Troubleshooting

**Web interface not accessible:**
- Check device IP address (printed in Serial)
- Verify firewall allows ports 80 and 81
- Try disabling WebSocket: `webUI.enableWebSocket(false)`

**WebSocket not connecting:**
- Check browser console for errors
- Verify port 81 is not blocked
- HTTP-only mode will work as fallback

**Changes not applying:**
- Wait 300ms after last change (auto-save debounce)
- Check Serial output for errors
- Try clicking "Apply Now" button manually

## Alternative Configuration Methods

If WebUI doesn't fit your needs, consider:

**Serial Commands:**
```cpp
// Implement your own serial config handler
if (Serial.available()) {
  String cmd = Serial.readStringUntil('\n');
  // Parse and apply configuration
}
```

**MQTT Configuration:**
```cpp
// Use PubSubClient library
mqtt.subscribe("fatamorgana/config");
// Handle messages and apply configuration
```

**REST API:**
```cpp
// Build custom REST endpoints
server.on("/api/setRect", HTTP_POST, handleSetRect);
```

## Compatibility

- ✅ ESP8266
- ✅ ESP32
- ❌ Arduino (no WiFi)
- ❌ Raspberry Pi Pico (no WiFi)
- ❌ Other platforms without WiFi/HTTP server

## Dependencies

- **FataMorgana** - Core protocol library (required)
- **ArduinoJson** - JSON parsing (^7.0.4)
- **WebSockets** - WebSocket server (^2.4.1)
- **ESP8266WebServer** / **WebServer** - HTTP server (built-in)

## License

MIT License - See LICENSE file

## Contributing

This is a helper library. If you need different configuration methods:
1. Use FataMorgana core library
2. Implement your own configuration interface
3. Share it as FataMorgana-MQTT, FataMorgana-BLE, etc.

## Support

For issues specific to WebUI, open an issue with `[WebUI]` prefix.
For core protocol issues, use the main FataMorgana repository.
