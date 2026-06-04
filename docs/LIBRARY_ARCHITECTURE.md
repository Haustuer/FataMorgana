# FataMorgana Library Architecture

## Overview

FataMorgana is split into **two separate libraries**:

1. **FataMorgana** - Core protocol library
2. **FataMorgana-WebUI** - Optional web configuration interface

## Why Two Libraries?

### Protocol vs Configuration

The **FataMorgana protocol** is about:
- UDP multicast frame distribution
- Binary packet format
- Discovery protocol
- LED rendering

The **Web Interface** is about:
- Device configuration
- Remote management
- Status monitoring

These are separate concerns and should not be coupled.

### Benefits of Separation

✅ **Minimal Core Dependencies**
- FataMorgana: Only needs Adafruit_NeoPixel
- No WebSocket/HTTP overhead if not needed

✅ **User Choice**
- Use WebUI if you want web configuration
- Skip it if you prefer Serial/MQTT/REST/etc.

✅ **Platform Flexibility**
- Core library could work on non-ESP platforms
- WebUI is ESP-specific

✅ **Cleaner Architecture**
- Protocol separate from configuration
- Each library has single responsibility

✅ **Easier Testing**
- Test protocol without web server
- Test WebUI independently

✅ **Extensibility**
- Easy to add FataMorgana-MQTT
- Easy to add FataMorgana-BLE
- Community can create alternatives

## Library Structure

```
lib/
├── FataMorgana/                    # CORE PROTOCOL
│   ├── src/
│   │   ├── FataMorgana.h           # Main header
│   │   ├── FataMorganaClient.h/cpp # Protocol client
│   │   ├── FataMorganaRenderer.h/cpp
│   │   ├── FataMorganaProtocol.h   # Constants
│   │   └── FataMorganaConfig.h     # Structures
│   ├── library.json
│   ├── library.properties
│   ├── keywords.txt
│   └── README.md
│
└── FataMorgana-WebUI/              # WEB INTERFACE HELPER
    ├── src/
    │   ├── FataMorganaWebUI.h/cpp  # Web server
    │   └── FataMorganaWebPage.h    # HTML page
    ├── library.json
    ├── library.properties
    ├── keywords.txt
    └── README.md
```

## Dependencies

### FataMorgana (Core)
```
Adafruit_NeoPixel  # LED control
```

### FataMorgana-WebUI (Optional)
```
FataMorgana        # Core library
ArduinoJson        # JSON parsing
WebSockets         # WebSocket server
```

## Usage Examples

### Core Only (Minimal)

```cpp
#include <FataMorgana.h>

FataMorganaClient client(256, 12);

void setup() {
  WiFi.begin("SSID", "PASSWORD");
  client.begin();
  client.setRectangle(0, 0, 16, 16);
}

void loop() {
  client.loop();
}
```

**Size:** ~50KB flash, ~2KB RAM

### With WebUI

```cpp
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>

FataMorganaClient client(256, 12);
FataMorganaWebUI webUI(client);

void setup() {
  WiFi.begin("SSID", "PASSWORD");
  client.begin();
  webUI.begin();
}

void loop() {
  client.loop();
  webUI.loop();
}
```

**Size:** ~65KB flash, ~6KB RAM

### Alternative: Serial Configuration

```cpp
#include <FataMorgana.h>

FataMorganaClient client(256, 12);

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");
  client.begin();
}

void loop() {
  client.loop();

  // Custom serial config
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.startsWith("rect ")) {
      // Parse and apply: rect 0 0 16 16
      int x, y, w, h;
      sscanf(cmd.c_str(), "rect %d %d %d %d", &x, &y, &w, &h);
      client.setRectangle(x, y, w, h);
    }
  }
}
```

### Alternative: MQTT Configuration (Future)

```cpp
#include <FataMorgana.h>
#include <FataMorgana-MQTT.h>  // Hypothetical future library

FataMorganaClient client(256, 12);
FataMorganaMQTT mqttConfig(client, "broker.local");

void setup() {
  WiFi.begin("SSID", "PASSWORD");
  client.begin();
  mqttConfig.begin();  // Subscribe to config topics
}

void loop() {
  client.loop();
  mqttConfig.loop();
}
```

## When to Use Each

### Use Core Only If:
- Fixed configuration (no runtime changes)
- Custom configuration method (Serial, MQTT, BLE)
- Minimal memory footprint needed
- Non-ESP platform (future)

### Use Core + WebUI If:
- Want web-based configuration
- Need remote management
- Non-technical users
- Real-time status monitoring

### Create Custom Helper If:
- Need MQTT configuration
- Need Bluetooth configuration
- Need REST API
- Need custom protocol

## Migration from Monolithic

### Before (Everything coupled):
```cpp
#include "main.h"  // 1000+ lines, everything mixed
```

### After (Clean separation):
```cpp
#include <FataMorgana.h>        // Protocol
#include <FataMorganaWebUI.h>  // Optional config
```

## Publishing

### PlatformIO Registry
```bash
# Publish separately
pio pkg publish lib/FataMorgana
pio pkg publish lib/FataMorgana-WebUI
```

### Arduino Library Manager
```bash
# Submit as separate libraries
# Users can install independently
```

## Future Extensions

Easy to add:
- **FataMorgana-MQTT** - MQTT configuration
- **FataMorgana-BLE** - Bluetooth Low Energy config
- **FataMorgana-REST** - REST API server
- **FataMorgana-Display** - OLED/LCD status display
- **FataMorgana-Button** - Physical button control

Community can create and share these!

## Design Principles

1. **Core stays minimal** - Only protocol essentials
2. **Helpers are optional** - User decides what to include
3. **Single responsibility** - Each library does one thing
4. **No circular dependencies** - Helpers depend on core, not vice versa
5. **Platform agnostic core** - Helpers can be platform-specific

## Comparison

| Aspect | Monolithic | Two Libraries |
|--------|-----------|---------------|
| Core size | 65KB | 50KB |
| Dependencies | 4 | 1 (core) |
| Flexibility | Low | High |
| User choice | None | Choose config method |
| Testability | Hard | Easy |
| Extensibility | Hard | Easy |
| Platform support | ESP only | Core: Any, Helpers: Specific |

## Summary

**FataMorgana = Protocol**
- What: UDP multicast LED display protocol
- Why: Synchronize frames across devices
- How: Binary packets, discovery, rendering

**FataMorgana-WebUI = Configuration**
- What: Web-based configuration interface
- Why: Easy remote configuration
- How: HTTP + WebSocket

**Result:** Clean, flexible, extensible architecture! 🎯
