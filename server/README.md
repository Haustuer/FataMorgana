# FataMorgana Server Components

This directory contains two server implementations for the FataMorgana LED display system.

## Components

### 1. gradientFrameServer.js

**Purpose:** UDP multicast frame broadcaster with web control panel

**Features:**
- **Web Control Panel** - Browser-based interface for device management
- **UDP Multicast** - Sends image frames to all devices (239.255.42.1:7777)
- **Device Discovery** - Auto-discover devices on network
- **Device Identification** - Flash specific devices to locate them physically
- **Image Upload** - Send custom images with auto-resizing
- **Test Patterns** - Gradient, solid, checkerboard, rainbow
- **Rainbow Animation** - Animated rainbow with configurable FPS (1-80), resolution, and encoding
- **Live Visualizer** - See frames with device regions overlaid
- **Coverage Map** - Calculate pixel coverage across devices
- Implements 8-byte header protocol (see PROJECT_IDEA.md)
- Supports RGB332 and RGB565 pixel formats
- Automatic frame chunking for large images
- Configurable inter-packet delay to prevent ESP WiFi buffer overflow

**Usage:**
```bash
# Install dependencies (if not already done)
npm install

# Start server with defaults
node gradientFrameServer.js

# With custom configuration
HTTP_PORT=3001 MULTICAST_ADDR=239.255.42.1 MULTICAST_PORT=7777 node gradientFrameServer.js

# Enable verbose logging
VERBOSE=true node gradientFrameServer.js
```

**Web Interface:**
Open `http://localhost:3001` for the control panel

**API Endpoints:**
- `GET /api/config` - Get server configuration and status
- `POST /api/frame` - Send test pattern frame
- `POST /api/frame/image` - Send custom image (base64 data URL)
- `POST /api/discover` - Discover all devices on network
- `POST /api/identify` - Identify specific device (flash LEDs)
- `GET /api/devices` - List all discovered devices
- `GET /api/coverage` - Calculate coverage for image dimensions
- `GET /api/frame/last` - Get last sent frame data

**Testing:**
```bash
# Discover devices
curl -X POST http://localhost:3001/api/discover

# Send gradient pattern
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{"width":24,"height":100,"pattern":"gradient","rgbType":0}'

# Identify device by IP
curl -X POST http://localhost:3001/api/identify \
  -H "Content-Type: application/json" \
  -d '{"ip":"192.168.1.100","flashCount":3}'

# Identify device by chip ID
curl -X POST http://localhost:3001/api/identify \
  -H "Content-Type: application/json" \
  -d '{"chipId":"FCB7D3","flashCount":5}'

# Send custom image
curl -X POST http://localhost:3001/api/frame/image \
  -H "Content-Type: application/json" \
  -d '{"image":"data:image/png;base64,...","targetWidth":24,"targetHeight":100,"rgbType":0}'
```

### 2. server.js

**Purpose:** WebSocket-based device management server

**Features:**
- Manages multiple ESP devices
- WebSocket communication for real-time updates
- Device registration and mapping
- Admin interface for device configuration
- Persists device-to-grid mapping

**Usage:**
```bash
# Start with default grid size
node server.js

# Custom grid configuration
GRID_X=8 GRID_Y=8 Z_LEN=100 PORT=8080 node server.js
```

**Endpoints:**
- `GET /api/config` - Grid configuration
- `GET /api/devices` - List connected devices
- `POST /api/assign` - Assign device to grid position
- `ws://host:port/ws/device` - WebSocket for ESP devices
- `ws://host:port/ws/admin` - WebSocket for admin UI

## Quick Start

### Running Both Servers

```bash
# Terminal 1: Device management server
node server.js

# Terminal 2: UDP frame broadcaster
node gradientFrameServer.js

# Terminal 3: Send test frames
node test-protocol.js gradient 24 100 0
```

### Environment Variables

**gradientFrameServer.js:**
- `HTTP_PORT` (default: 3001) - HTTP API and web interface port
- `MULTICAST_ADDR` (default: 239.255.42.1) - UDP multicast group address
- `MULTICAST_PORT` (default: 7777) - UDP multicast port
- `RESPONSE_PORT` (default: 7778) - UDP port for device responses (discovery)
- `MULTICAST_TTL` (default: 1) - Multicast TTL (1=local subnet)
- `INTER_PACKET_DELAY_MS` (default: 2) - Delay between UDP packets
- `DISCOVERY_TIMEOUT_MS` (default: 2000) - Discovery timeout
- `VERBOSE` (default: false) - Enable detailed logging

**server.js:**
- `PORT` (default: 8080) - HTTP/WebSocket port
- `GRID_X` (default: 6) - Grid width
- `GRID_Y` (default: 6) - Grid height
- `Z_LEN` (default: 100) - Default Z-axis LED count
- `VERBOSE_JSON` (default: false) - Log all JSON messages

## Protocol Documentation

See **PROTOCOL_IMPLEMENTATION.md** for detailed protocol specification and usage examples.

## File Structure

```
server/
├── gradientFrameServer.js          # UDP multicast frame broadcaster
├── gradient-frame-public/          # Web control panel
│   └── index.html                 # Single-page control interface
├── server.js                       # WebSocket device manager (legacy)
├── logger.js                       # Logging utility
├── test-protocol.js                # Protocol testing script
├── PROTOCOL_IMPLEMENTATION.md      # Detailed protocol docs
├── README.md                       # This file
├── package.json                    # Node dependencies
├── public/                         # Admin web interface (legacy)
└── data/                          # Persistent data (created at runtime)
    └── mapping.json               # Device mappings
```

## Network Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                          Your Network                           │
│                                                                 │
│  ┌──────────────────────────────────────────────┐              │
│  │  gradientFrameServer.js                      │              │
│  │  ┌──────────────┐  ┌────────────────────┐   │              │
│  │  │ Web UI       │  │ UDP Multicast      │   │              │
│  │  │ Port 3001    │  │ 239.255.42.1:7777  │   │              │
│  │  └──────────────┘  └────────────────────┘   │              │
│  └──────┬───────────────────┬───────────────────┘              │
│         │                   │ Multicast                        │
│         │ HTTP              │ (Discovery, Identify, Frames)    │
│         │                   ▼                                  │
│  ┌──────▼────────────────────────────────────────────┐        │
│  │           ESP8266/ESP32 Devices                   │        │
│  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐         │        │
│  │  │ ESP1 │  │ ESP2 │  │ ESP3 │  │ ESP4 │         │        │
│  │  │ :100 │  │ :104 │  │ :106 │  │ :107 │         │        │
│  │  └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘         │        │
│  │     │         │         │         │              │        │
│  │     │ Unicast responses (port 7778)              │        │
│  │     └─────────┴─────────┴─────────┴──────────────┤        │
│  │                                                   │        │
│  │     ▼         ▼         ▼         ▼              │        │
│  │  ┌─────────────────────────────────────────┐    │        │
│  │  │       NeoPixel LED Strips/Matrices      │    │        │
│  │  │  Row 0     16x16    16x16     16x16     │    │        │
│  │  └─────────────────────────────────────────┘    │        │
│  └───────────────────────────────────────────────────┘        │
│                                                                 │
│  Features:                                                     │
│  • Discover: Server finds all devices automatically           │
│  • Identify: Flash specific device LEDs to locate             │
│  • Multicast: One frame sent to all devices simultaneously    │
│  • Coverage: Visualize which devices cover which pixels       │
└─────────────────────────────────────────────────────────────────┘
```

## Development

### Adding New Patterns

Edit `gradientFrameServer.js` and add a new pattern generator function:

```javascript
function buildMyPatternFrame(width, height, rgbType, options) {
  const payload = Buffer.alloc(width * height * bytesPerPixel(rgbType));

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const pixelIndex = y * width + x;

      // Your pattern logic here
      const r = ..., g = ..., b = ...;

      if (rgbType === RGB565) {
        payload.writeUInt16LE(encodeRgb565Pixel(r, g, b), pixelIndex * 2);
      } else {
        payload[pixelIndex] = encodeRgb332Pixel(r, g, b);
      }
    }
  }

  return payload;
}
```

Then add it to the pattern switch in `sendFrame()`.

### Debugging

**Enable verbose logging:**
```bash
VERBOSE=true node gradientFrameServer.js
```

**Check ESP status:**
```bash
# Assuming ESP at 192.168.1.100
curl http://192.168.1.100/status.json
```

**Monitor packets with tcpdump:**
```bash
sudo tcpdump -i any -n udp port 7777 -X
```

## Troubleshooting

### ESP Not Receiving Frames

1. Verify multicast is working: `MULTICAST_ADDR=239.255.42.1`
2. Check ESP is on same subnet
3. Check firewall allows UDP ports 7777 (multicast) and 7778 (responses)
4. Enable IGMP Snooping on router
5. Test discovery: `curl -X POST http://localhost:3001/api/discover`
6. Increase inter-packet delay: `INTER_PACKET_DELAY_MS=5`

### Discovery Not Finding Devices

1. Check devices are connected to WiFi
2. Verify firewall allows UDP port 7778
3. Check router supports multicast/IGMP
4. Monitor server logs for responses
5. Try manual device access: `http://<device-ip>`

### Incomplete/Corrupted Frames

1. Reduce frame size
2. Increase inter-packet delay
3. Check WiFi signal strength
4. Monitor ESP serial output for errors

### Performance Issues

1. Use RGB332 instead of RGB565 (50% less bandwidth)
2. Reduce frame rate
3. Use smaller image dimensions
4. Check network congestion

## Contributing

When modifying the protocol:

1. Update PROJECT_IDEA.md (protocol design)
2. Update PROTOCOL_IMPLEMENTATION.md (implementation details)
3. Update ESP code in src/main.cpp
4. Test with multiple ESP devices
5. Document breaking changes

## License

See main project LICENSE file.
