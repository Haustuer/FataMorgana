# UDP Protocol Implementation Guide

## Overview

This document describes the UDP frame protocol implementation for the FataMorgana LED display system.

## Protocol Specification

### Header Format (8 bytes)

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | FrameType | uint8 | `0` = Config, `1` = Image Start, `2` = Image Continuation |
| 1 | FrameCounter | uint8 | Sequence number (0-255, wraps around) |
| 2 | ChunkIndex | uint8 | Zero-based chunk index within frame |
| 3 | RGBType | uint8 | `0` = RGB332, `1` = RGB565 |
| 4-5 | Width | uint16 LE | Image width in pixels (little-endian) |
| 6-7 | Height | uint16 LE | Image height in pixels (little-endian) |

### Payload Format

After the 8-byte header, the payload contains pixel data in row-major order:
- Row 0: pixels from left to right
- Row 1: pixels from left to right
- ... and so on

### RGB Encoding

**RGB332 (1 byte per pixel):**
```
Bit layout: RRRGGGBB
- Red:   3 bits (bits 7-5)
- Green: 3 bits (bits 4-2)
- Blue:  2 bits (bits 1-0)
```

**RGB565 (2 bytes per pixel, little-endian):**
```
Bit layout: RRRRRGGGGGGBBBBB
- Red:   5 bits (bits 15-11)
- Green: 6 bits (bits 10-5)
- Blue:  5 bits (bits 4-0)

Byte order: LSB first (little-endian)
Example: 0xF800 (red) is sent as [0x00, 0xF8]
```

## Implementation: gradientFrameServer.js

### Features

1. **Multiple Pattern Types**
   - Gradient: R/G/B gradients based on position
   - Solid: Single color fill
   - Checkerboard: Alternating black/white cells
   - Rainbow: HSV-based rainbow pattern

2. **Inter-Packet Delay**
   - Configurable delay between UDP packets (default 2ms)
   - Prevents overwhelming ESP8266 WiFi receive buffer
   - Set via `INTER_PACKET_DELAY_MS` environment variable

3. **Chunked Transfer**
   - Automatically splits large frames into multiple UDP packets
   - Max payload per packet: 1192 bytes (1200 total - 8 byte header)
   - Max total frame size: 4800 bytes

4. **Verbose Logging**
   - Enable with `VERBOSE=true` environment variable
   - Logs each packet transmission with details

### Usage Examples

#### Starting the Server

```bash
# Basic usage
node gradientFrameServer.js

# With custom settings
HTTP_PORT=3001 UDP_PORT=7777 BROADCAST_ADDR=192.168.1.255 node gradientFrameServer.js

# With verbose logging
VERBOSE=true node gradientFrameServer.js

# With custom inter-packet delay (5ms)
INTER_PACKET_DELAY_MS=5 node gradientFrameServer.js
```

#### Sending Frames via HTTP API

**Get Configuration:**
```bash
curl http://localhost:3001/api/config
```

**Send Gradient Frame:**
```bash
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{"width":24,"height":100,"rgbType":0,"pattern":"gradient"}'
```

**Send Solid Red Frame:**
```bash
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{
    "width":16,
    "height":16,
    "rgbType":0,
    "pattern":"solid",
    "patternOptions":{"r":255,"g":0,"b":0}
  }'
```

**Send Checkerboard:**
```bash
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{
    "width":16,
    "height":16,
    "rgbType":0,
    "pattern":"checkerboard",
    "patternOptions":{"cellSize":4}
  }'
```

**Send Rainbow:**
```bash
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{
    "width":24,
    "height":24,
    "rgbType":1,
    "pattern":"rainbow",
    "patternOptions":{"offset":0}
  }'
```

### API Request Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| width | number | No | 24 | Image width (1-65535) |
| height | number | No | 100 | Image height (1-65535) |
| rgbType | number | No | 0 | `0` for RGB332, `1` for RGB565 |
| pattern | string | No | "gradient" | Pattern type (see below) |
| patternOptions | object | No | {} | Pattern-specific options |

### Pattern Options

**gradient:** No options

**solid:**
- `r` (number, 0-255): Red value
- `g` (number, 0-255): Green value
- `b` (number, 0-255): Blue value

**checkerboard:**
- `cellSize` (number): Size of each checker cell in pixels

**rainbow:**
- `offset` (number): Offset for animation

## ESP8266 Implementation

The ESP code in `src/main.cpp` implements the receiver side:

1. Listens for UDP packets on port 7777
2. Parses 8-byte header
3. Reconstructs frames from chunks
4. Maps frame regions to LED strips
5. Renders to NeoPixel hardware

### Frame Reception State Machine

```
IDLE --> IMAGE_START received --> IN_PROGRESS
                                       |
                            Collect CONTINUATION chunks
                                       |
                            All chunks received?
                                       |
                                    Render
                                       |
                                    IDLE
```

### Timeout Handling

Currently, incomplete frames remain in memory until overwritten by a new IMAGE_START packet. Consider implementing timeout logic to clear stale frames.

## Performance Considerations

### Packet Loss
- UDP does not guarantee delivery
- Lost packets result in incomplete frames
- ESP will not render until all chunks received
- Solution: Retransmit entire frame on timeout (not yet implemented)

### WiFi Buffer Overflow
- ESP8266 has limited WiFi RX buffer (~5-6 KB)
- Sending packets too fast can overflow buffer
- Use `INTER_PACKET_DELAY_MS` to throttle transmission
- Recommended: 2-5ms delay between packets

### Frame Rate
- Depends on frame size and inter-packet delay
- Example (24x100, RGB332, 2ms delay):
  - Payload: 2400 bytes
  - Chunks: 3 packets
  - Transmission time: ~6ms + network latency
  - Max FPS: ~160 fps (practically limited by ESP processing)

### Size Limits

**RGB332:**
- Max pixels per packet: 1192
- Max total pixels: 4800
- Example: 69x69 grid = 4761 pixels (fits)

**RGB565:**
- Max pixels per packet: 596
- Max total pixels: 2400
- Example: 48x48 grid = 2304 pixels (fits)

## Troubleshooting

### ESP Not Receiving Frames

1. Check broadcast address matches your network subnet
2. Verify UDP port 7777 is not blocked by firewall
3. Enable verbose logging: `VERBOSE=true`
4. Check ESP serial output for rejection messages

### Incomplete Frames

1. Increase inter-packet delay: `INTER_PACKET_DELAY_MS=5`
2. Reduce frame size
3. Check network congestion
4. Monitor ESP rejected packet counter (via web status page)

### Wrong Colors

1. Verify RGB encoding matches between sender/receiver
2. Check endianness (RGB565 uses little-endian)
3. Test with solid color patterns to verify encoding

## Future Improvements

1. **Config Frames (Type 0)**
   - Send mapping configuration via UDP
   - Dynamic LED count/layout updates

2. **Acknowledgment Protocol**
   - ESP sends ACK after frame completion
   - Server can detect packet loss

3. **Compression**
   - RLE or delta encoding for animation
   - Reduce bandwidth for static/slow-changing content

4. **Multicast Support**
   - Target specific ESP devices
   - Reduce network traffic

5. **Frame Timing**
   - Synchronized playback across multiple ESPs
   - Timestamp in header for coordination

## Reference

See also:
- `PROJECT_IDEA.md` - Original protocol design document
- `src/main.cpp` - ESP8266 receiver implementation
- `server/server.js` - WebSocket-based device management server
