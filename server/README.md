# FataMorgana Server Components

This directory contains two server implementations for the FataMorgana LED display system.

## Components

### 1. gradientFrameServer.js

**Purpose:** UDP frame broadcaster implementing the FataMorgana protocol

**Features:**
- Sends image frames via UDP broadcast to ESP8266 devices
- Implements 8-byte header protocol (see PROJECT_IDEA.md)
- Supports RGB332 and RGB565 pixel formats
- Multiple test patterns: gradient, solid, checkerboard, rainbow
- Automatic frame chunking for large images
- Configurable inter-packet delay to prevent ESP WiFi buffer overflow

**Usage:**
```bash
# Install dependencies (if not already done)
npm install

# Start server with defaults
node gradientFrameServer.js

# With custom configuration
HTTP_PORT=3001 UDP_PORT=7777 BROADCAST_ADDR=192.168.1.255 node gradientFrameServer.js

# Enable verbose logging
VERBOSE=true node gradientFrameServer.js
```

**API Endpoints:**
- `GET /api/config` - Get server configuration and status
- `POST /api/frame` - Send a frame (see PROTOCOL_IMPLEMENTATION.md)

**Testing:**
```bash
# Quick test
node test-protocol.js gradient 16 16 0

# Solid red frame
node test-protocol.js solid 24 100 0 255 0 0

# Large RGB565 rainbow
node test-protocol.js rainbow 48 48 1
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
- `HTTP_PORT` (default: 3001) - HTTP API port
- `UDP_PORT` (default: 7777) - UDP broadcast port
- `BROADCAST_ADDR` (default: 255.255.255.255) - Broadcast address
- `INTER_PACKET_DELAY_MS` (default: 2) - Delay between UDP packets
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
├── gradientFrameServer.js      # UDP frame broadcaster
├── server.js                   # WebSocket device manager
├── logger.js                   # Logging utility
├── test-protocol.js            # Protocol testing script
├── PROTOCOL_IMPLEMENTATION.md  # Detailed protocol docs
├── README.md                   # This file
├── package.json               # Node dependencies
├── public/                    # Admin web interface
└── data/                      # Persistent data (created at runtime)
    └── mapping.json           # Device mappings
```

## Network Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     Your Network                        │
│                                                         │
│  ┌──────────────────┐         ┌──────────────────┐    │
│  │ server.js        │         │ gradientFrame    │    │
│  │ (WebSocket)      │         │ Server.js        │    │
│  │ Port 8080        │         │ (UDP Broadcast)  │    │
│  └────────┬─────────┘         └────────┬─────────┘    │
│           │                            │               │
│           │ WS                         │ UDP           │
│           │                            │ Port 7777     │
│           ▼                            ▼               │
│  ┌─────────────────────────────────────────────┐      │
│  │           ESP8266 Devices                   │      │
│  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐   │      │
│  │  │ ESP1 │  │ ESP2 │  │ ESP3 │  │ ESP4 │   │      │
│  │  └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘   │      │
│  │     │         │         │         │        │      │
│  │     ▼         ▼         ▼         ▼        │      │
│  │  ┌──────────────────────────────────────┐ │      │
│  │  │       NeoPixel LED Strips           │ │      │
│  │  └──────────────────────────────────────┘ │      │
│  └─────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────┘
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

1. Check broadcast address: `BROADCAST_ADDR=192.168.1.255`
2. Verify ESP is on same subnet
3. Check firewall allows UDP port 7777
4. Increase inter-packet delay: `INTER_PACKET_DELAY_MS=5`

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
