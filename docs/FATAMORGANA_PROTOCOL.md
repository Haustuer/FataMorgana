# FataMorgana Protocol Specification

**Version:** 1.0
**Date:** 2024-01-20
**Status:** Draft

**Network Model:** Multicast (Server → Devices) + Unicast (Devices → Server)

## 1. Introduction

### 1.1 Purpose

The FataMorgana Protocol is a UDP-based communication protocol designed for distributed LED display systems. It enables a central server to broadcast image frames to multiple ESP8266-based receivers, each displaying a configurable region of the image on connected LED strips.

### 1.2 Goals

- **Simple**: Minimal overhead, easy to implement on embedded devices
- **Fast**: Low latency for real-time animations
- **Flexible**: Support various pixel formats and display configurations
- **Discoverable**: Automatic device discovery and mapping
- **Extensible**: Room for future protocol enhancements

### 1.3 Use Cases

- Large-scale LED installations with multiple controllers
- Distributed pixel displays across rooms or buildings
- Synchronized LED art installations
- Interactive light shows

## 2. Protocol Overview

### 2.1 Architecture

```
┌────────────┐                      ┌─────────────┐
│   Server   │  ─── UDP 7777 ───>   │  ESP Client │
│            │    (multicast)       │   (Device)  │
│            │  <── UDP 7778 ───    │             │
└────────────┘    (unicast)         └─────────────┘
```

**Communication:**
- Server sends to multicast group 239.255.42.1:7777 (all devices listen)
- Devices respond via unicast to server IP on port 7778
- Image frames use multicast to reach all devices efficiently
- Discovery responses use unicast for reliable delivery
- No persistent connections (stateless UDP)

### 2.2 Packet Types

| Type | Name | Direction | Description |
|------|------|-----------|-------------|
| 0 | Config Frame | Server → Device | Discovery and configuration |
| 1 | Image Start | Server → Device | First chunk of image frame |
| 2 | Image Continue | Server → Device | Continuation chunk of image frame |

## 3. Common Header Format

All FataMorgana packets begin with an 8-byte header:

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   FrameType   | FrameCounter  |  ChunkIndex   |   TypeData    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            Field1             |            Field2             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 3.1 Header Fields

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | FrameType | uint8 | Packet type (0=Config, 1=ImageStart, 2=ImageContinue) |
| 1 | FrameCounter | uint8 | Sequence number (0-255, wraps) |
| 2 | ChunkIndex | uint8 | Zero-based chunk index within frame |
| 3 | TypeData | uint8 | Type-specific data (meaning varies by FrameType) |
| 4-5 | Field1 | uint16 | Type-specific field (little-endian) |
| 6-7 | Field2 | uint16 | Type-specific field (little-endian) |

**Note:** All multi-byte integers use **little-endian** byte order.

## 4. Config Frame (Type 0)

### 4.1 Purpose

Config frames enable device discovery and future configuration capabilities.

### 4.2 Header Format

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Type=0       | FrameCounter  | ChunkIndex=0  |    SubType    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Reserved=0           |          Reserved=0           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 4.3 SubType Values

| Value | Name | Description |
|-------|------|-------------|
| 0 | DISCOVERY | Request device information and configuration |
| 1 | SET_MAPPING | Set device mapping (reserved for future) |
| 2 | SET_BRIGHTNESS | Set LED brightness (reserved for future) |
| 3-255 | - | Reserved for future use |

### 4.4 SubType 0: Discovery Request

**Purpose:** Server broadcasts to discover all devices and their configurations.

**Packet Structure:**
```
Header (8 bytes): Type=0, SubType=0
Payload (6 bytes):
  Bytes 8-11:  Server IPv4 address (network byte order, big-endian)
  Bytes 12-13: Response UDP port (network byte order, big-endian)
```

**Total Size:** 14 bytes

**Example:**
```
Server IP: 192.168.1.10
Response Port: 7778

Hex dump:
00 01 00 00 00 00 00 00    Header
C0 A8 01 0A 1E 62          Payload (192.168.1.10 : 7778)
```

### 4.5 Discovery Response

**Direction:** Device → Server
**Transport:** UDP packet to server IP on response port
**Format:** Binary (64 bytes fixed)
**Magic Bytes:** `0x46415441` ("FATA")

**Binary Structure:**
```
Byte Range | Field                    | Type      | Byte Order
-----------|--------------------------|-----------|------------------
0-3        | Magic Bytes              | uint32    | BE (0x46415441)
4          | Protocol Version         | uint8     | -
5          | Response Type            | uint8     | 0x01
6-7        | Reserved                 | uint16    | -
8-11       | Device IP                | uint32    | BE (network order)
12-17      | MAC Address              | uint8[6]  | -
18-21      | Chip ID                  | uint32    | LE
22-25      | Uptime (ms)              | uint32    | LE
26-27      | LED Count                | uint16    | LE
28         | LED Pin                  | uint8     | -
29         | Brightness               | uint8     | -
30         | Firmware Major           | uint8     | -
31         | Firmware Minor           | uint8     | -
32         | Mapping Mode             | uint8     | -
33         | Sample Mode              | uint8     | -
34-35      | Row/Column Index         | uint16    | LE
36-37      | Line Pixels              | uint16    | LE
38-39      | Rect X                   | uint16    | LE
40-41      | Rect Y                   | uint16    | LE
42-43      | Rect Width               | uint16    | LE
44-45      | Rect Height              | uint16    | LE
46         | Serpentine               | uint8     | -
47         | Reserved                 | uint8     | -
48-51      | Accepted Packets         | uint32    | LE
52-55      | Rejected Packets         | uint32    | LE
56-59      | Rendered Frames          | uint32    | LE
60-61      | Last Frame Width         | uint16    | LE
62-63      | Last Frame Height        | uint16    | LE
```

**Total Size:** 64 bytes (fixed)

**Example Packet:**
```
46 41 54 41 01 01 00 00  |  Magic "FATA", Ver 1, Type 1
C0 A8 01 64 AA BB CC DD  |  IP 192.168.1.100, MAC start
EE FF 12 34 AB CD 00 0E  |  MAC end, ChipID 0x1234ABCD
10 00 01 00 0C 50 01 00  |  Uptime, LEDs:256, Pin:12, Bright:80, FW:1.0
02 00 00 00 10 00 00 00  |  Mode:2, Sample:0, RowIdx:0, LinePx:16
00 00 10 00 10 00 00 00  |  RectX:0, RectY:0, RectW:16, RectH:16, Serp:0
00 05 F3 00 00 00 0C 00  |  AcceptedPkts:1523, RejectedPkts:12
00 01 FB 00 18 00 64 00  |  RenderedFrames:507, LastW:24, LastH:100
```

**Advantages:**
- 78% smaller than JSON (~300 bytes → 64 bytes)
- Fixed-size simplifies parsing
- No ArduinoJson dependency on ESP
- Consistent with binary protocol philosophy
- Magic bytes prevent false positives

### 4.6 Response Field Definitions

#### device
- **ip** - Device IPv4 address (string)
- **mac** - MAC address in colon format (string)
- **hostname** - Device hostname (string)
- **chipId** - ESP chip ID in hex (string)
- **firmware** - Firmware version (string)
- **uptime** - Milliseconds since boot (number)

#### hardware
- **ledCount** - Total number of LEDs (number)
- **ledPin** - GPIO pin for LED data (number)
- **brightness** - Current brightness 0-255 (number)
- **ledType** - LED chip type (string)

#### mapping
- **mode** - Display mode (number):
  - `0` = ROW - Display single row from image
  - `1` = COLUMN - Display single column from image
  - `2` = RECTANGLE - Display rectangular region
- **modeName** - Human-readable mode (string)
- **sampleMode** - Sampling method (number):
  - `0` = PIXEL - Nearest neighbor
  - `1` = INTERPOLATED - Linear interpolation
- **sampleModeName** - Human-readable sample mode (string)
- **rowIndex** - Row index when mode=0 (number)
- **columnIndex** - Column index when mode=1 (number)
- **linePixels** - LED count for row/column mode (number)
- **rectX** - Rectangle X offset when mode=2 (number)
- **rectY** - Rectangle Y offset when mode=2 (number)
- **rectWidth** - Rectangle width when mode=2 (number)
- **rectHeight** - Rectangle height when mode=2 (number)
- **serpentine** - Serpentine wiring layout (boolean)

#### status
- **lastFrameCounter** - Last received frame counter (number)
- **lastFrameWidth** - Last frame width (number)
- **lastFrameHeight** - Last frame height (number)
- **lastFrameRgbType** - Last RGB type (number)
- **lastFrameMillis** - Timestamp of last frame (number)
- **acceptedPackets** - Total accepted packets (number)
- **rejectedPackets** - Total rejected packets (number)
- **renderedFrames** - Total rendered frames (number)

## 5. Image Frame (Type 1 & 2)

### 5.1 Purpose

Image frames transmit pixel data from server to devices for display on LED strips.

### 5.2 Header Format

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Type=1 or 2   | FrameCounter  |  ChunkIndex   |    RGBType    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             Width             |            Height             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 5.3 Header Fields

| Byte | Field | Description |
|------|-------|-------------|
| 0 | FrameType | `1` = first chunk, `2` = continuation chunk |
| 1 | FrameCounter | Image sequence number (wraps at 255) |
| 2 | ChunkIndex | Zero-based index of this chunk |
| 3 | RGBType | Pixel format: `0`=RGB332, `1`=RGB565 |
| 4-5 | Width | Image width in pixels (little-endian) |
| 6-7 | Height | Image height in pixels (little-endian) |

### 5.4 Payload Format

**Pixel Data:** Raw pixel bytes in row-major order

```
Row 0: [pixel 0,0] [pixel 0,1] [pixel 0,2] ...
Row 1: [pixel 1,0] [pixel 1,1] [pixel 1,2] ...
Row 2: [pixel 2,0] [pixel 2,1] [pixel 2,2] ...
...
```

### 5.5 RGB Encoding

#### RGB332 (1 byte per pixel)

```
Bit layout: RRRGGGBB

 7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ R │ R │ R │ G │ G │ G │ B │ B │
└───┴───┴───┴───┴───┴───┴───┴───┘

Red:   3 bits (values 0-7) → scale to 0-255
Green: 3 bits (values 0-7) → scale to 0-255
Blue:  2 bits (values 0-3) → scale to 0-255
```

**Encoding:**
```c
uint8_t encoded = (r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6);
```

**Decoding:**
```c
uint8_t r = ((encoded >> 5) & 0x07) * 255 / 7;
uint8_t g = ((encoded >> 2) & 0x07) * 255 / 7;
uint8_t b = (encoded & 0x03) * 255 / 3;
```

#### RGB565 (2 bytes per pixel, little-endian)

```
Bit layout: RRRRRGGGGGGBBBBB

15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ R │ R │ R │ R │ R │ G │ G │ G │ G │ G │ G │ B │ B │ B │ B │ B │
└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘

Red:   5 bits (values 0-31) → scale to 0-255
Green: 6 bits (values 0-63) → scale to 0-255
Blue:  5 bits (values 0-31) → scale to 0-255

Byte order: Little-endian (LSB first)
```

**Encoding:**
```c
uint16_t encoded = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
buffer[offset] = encoded & 0xFF;        // LSB first
buffer[offset+1] = (encoded >> 8) & 0xFF;
```

**Decoding:**
```c
uint16_t encoded = buffer[offset] | (buffer[offset+1] << 8);
uint8_t r = ((encoded >> 11) & 0x1F) * 255 / 31;
uint8_t g = ((encoded >> 5) & 0x3F) * 255 / 63;
uint8_t b = (encoded & 0x1F) * 255 / 31;
```

### 5.6 Chunking

Large frames are split into multiple UDP packets:

**Constants:**
- `MAX_PACKET_SIZE` = 1200 bytes (safe for WiFi without fragmentation)
- `HEADER_SIZE` = 8 bytes
- `MAX_PAYLOAD_SIZE` = 1192 bytes (1200 - 8)
- `MAX_FRAME_BYTES` = 4800 bytes (device buffer limit)

**Chunk Count:**
```
chunkCount = ceil(totalFrameBytes / MAX_PAYLOAD_SIZE)
```

**Frame Size Limits:**
- RGB332: max 4800 pixels (e.g., 69x69 grid)
- RGB565: max 2400 pixels (e.g., 48x48 grid)

### 5.7 Frame Transmission

**Type 1 (Image Start):**
- First packet of a new frame
- `FrameCounter` increments (wraps at 255)
- `ChunkIndex` = 0
- Receiver initializes frame buffer

**Type 2 (Image Continue):**
- Subsequent packets of same frame
- `FrameCounter` matches Type 1 packet
- `ChunkIndex` = 1, 2, 3, ...
- Receiver appends to frame buffer

**Example Sequence:**
```
Frame #5, 32x32 RGB332 (1024 bytes, 1 chunk):
  Packet: Type=1, Counter=5, Chunk=0, RGB=0, 32x32, [1024 bytes]

Frame #6, 48x48 RGB565 (4608 bytes, 4 chunks):
  Packet 1: Type=1, Counter=6, Chunk=0, RGB=1, 48x48, [1192 bytes]
  Packet 2: Type=2, Counter=6, Chunk=1, RGB=1, 48x48, [1192 bytes]
  Packet 3: Type=2, Counter=6, Chunk=2, RGB=1, 48x48, [1192 bytes]
  Packet 4: Type=2, Counter=6, Chunk=3, RGB=1, 48x48, [1032 bytes]
```

## 6. Network Configuration

### 6.1 UDP Ports

| Port | Purpose | Direction |
|------|---------|-----------|
| 7777 | Frame transmission | Server → Device (multicast) |
| 7778 | Discovery responses | Device → Server (unicast) |

### 6.2 Multicast Group

**Address:** `239.255.42.1` (IPv4 site-local multicast)
**Port:** `7777`

**Server:**
- Sends all frames (Type 0, 1, 2) to multicast group
- Joins multicast group not required (send-only)
- Listens for unicast responses on port 7778

**Devices:**
- Must join multicast group `239.255.42.1` on startup
- Listen on port 7777 for multicast packets
- Send responses via unicast to server IP on port 7778

**Why Multicast?**
- More efficient than broadcast (targeted delivery)
- Reduces network noise
- Router/switch can optimize delivery
- Devices opt-in by joining group

**Multicast Range:**
- `239.0.0.0/8` is reserved for site-local multicast
- `239.255.42.1` chosen to avoid conflicts
- Can be configured via environment variable if needed

### 6.3 Firewall Requirements

Allow UDP traffic:
- **Server outbound:** Port 7777 to 239.255.42.1 (multicast)
- **Server inbound:** Port 7778 from any (unicast responses)
- **Device inbound:** Port 7777 from multicast group
- **Device outbound:** Port 7778 to server IP (unicast)

### 6.4 Router/Switch Configuration

**IGMP Snooping:**
- Should be enabled for optimal multicast delivery
- If disabled, multicast acts like broadcast (still works)

**Multicast Routing:**
- Not required (single subnet deployment)
- If crossing subnets, enable PIM or static multicast routes

**WiFi Considerations:**
- Some WiFi routers drop multicast by default
- Enable "IGMP Proxy" or "Multicast forwarding" in router settings
- Test with: `ping 239.255.42.1` from server

## 7. Device Behavior

### 7.1 Frame Reception State Machine

```
State: IDLE
  ↓
Receive Type 1 (Image Start)
  ↓
Initialize frame buffer
Set expected chunk count
  ↓
State: RECEIVING
  ↓
Receive Type 2 packets
Validate frame counter matches
Store chunks
  ↓
All chunks received?
  ↓ Yes
Render frame to LEDs
  ↓
State: IDLE
```

### 7.2 Error Handling

**Invalid Packet:**
- Packet too small (< 8 bytes) → Discard
- Unknown frame type → Discard
- Invalid RGB type → Discard

**Frame Errors:**
- Chunk index out of range → Discard packet
- Frame counter mismatch during receive → Discard packet
- Duplicate chunk → Accept (overwrite previous)
- Frame too large for buffer → Reject entire frame

**Timeouts:**
- Incomplete frame after timeout → Discard frame (recommended: 1 second)
- No frames for extended period → Keep last rendered frame

### 7.3 Region Mapping

Each device extracts a specific region from the received image based on its configured mapping mode:

**ROW Mode:**
```
Extract row 'rowIndex'
Sample 'linePixels' LEDs from that row
Apply sample mode (pixel or interpolated)
```

**COLUMN Mode:**
```
Extract column 'columnIndex'
Sample 'linePixels' LEDs from that column
Apply sample mode (pixel or interpolated)
```

**RECTANGLE Mode:**
```
Extract rectangle from (rectX, rectY) to (rectX+rectWidth-1, rectY+rectHeight-1)
Map pixels to LED strip in row-major order
If serpentine=true, reverse every other row
```

## 8. Performance Considerations

### 8.1 Packet Timing

**Inter-packet Delay:**
- Recommended: 2-5ms between packets
- Prevents ESP WiFi buffer overflow
- Balances throughput vs reliability

**Frame Rate:**
```
Example: 24x100 RGB332 (2400 bytes)
  Chunks: 3 packets
  Inter-packet delay: 2ms
  Transmission time: ~6ms + network latency
  Theoretical max: ~160 fps
  Practical limit: 30-60 fps (ESP processing)
```

### 8.2 Bandwidth

```
RGB332: width * height * 1 byte
RGB565: width * height * 2 bytes

Add 8 bytes header per chunk
Add inter-packet delays
Add network overhead (~28 bytes UDP/IP headers)

Example: 48x48 RGB565 @ 30fps
  Frame: 4608 bytes
  Chunks: 4 packets
  Total per frame: 4800 bytes (with headers)
  Bandwidth: 144 KB/s = 1.15 Mbps
```

### 8.3 Packet Loss

UDP does not guarantee delivery:
- Lost Type 1 packet → Frame never starts
- Lost Type 2 packet → Incomplete frame, no render
- No automatic retransmission
- Implement timeout to clear incomplete frames

**Mitigation:**
- Use wired Ethernet where possible
- Strong WiFi signal (RSSI > -70 dBm)
- Minimize network congestion
- Consider frame retransmission on timeout

## 9. Discovery Process

### 9.1 Discovery Workflow

```
1. Server: Send discovery request (Type 0, SubType 0)
   └─> UDP multicast to 239.255.42.1:7777
   └─> Packet contains server IP and response port

2. Devices: Receive discovery request (multicast listener)
   └─> Parse server IP and response port from payload
   └─> Build JSON response with device info and mapping
   └─> Send unicast UDP to server IP:response_port

3. Server: Collect unicast responses (2 second timeout)
   └─> Listen on port 7778 for incoming packets
   └─> Parse JSON from each device
   └─> Build device map
   └─> Calculate coverage

4. Server: Store device configurations
   └─> Make available via HTTP API
   └─> Display coverage visualization
```

**Key Points:**
- Discovery request uses **multicast** (efficient, targeted)
- Responses use **unicast** (reliable, no broadcast storm)
- Devices must join multicast group on boot
- Server must parse server IP from its own packet payload

### 9.2 Coverage Calculation

For each device, determine which image pixels it uses:

```javascript
function calculateCoverage(device, imageWidth, imageHeight) {
  const { mapping } = device;

  if (mapping.mode === 0) { // ROW
    return {
      x: 0,
      y: mapping.rowIndex,
      width: imageWidth,
      height: 1,
      pixels: mapping.linePixels
    };
  }

  if (mapping.mode === 1) { // COLUMN
    return {
      x: mapping.columnIndex,
      y: 0,
      width: 1,
      height: imageHeight,
      pixels: mapping.linePixels
    };
  }

  if (mapping.mode === 2) { // RECTANGLE
    return {
      x: mapping.rectX,
      y: mapping.rectY,
      width: mapping.rectWidth,
      height: mapping.rectHeight,
      pixels: mapping.rectWidth * mapping.rectHeight
    };
  }
}
```

### 9.3 Visualization

Server can display:
- Which devices are online
- What region each displays
- Coverage gaps (image areas with no device)
- Coverage overlaps (multiple devices showing same area)

## 10. Future Extensions

### 10.1 Planned Features

**Config SubType 1: SET_MAPPING**
- Remotely configure device mapping
- Avoid need to connect to ESP web interface

**Config SubType 2: SET_BRIGHTNESS**
- Adjust brightness centrally
- Synchronize brightness across all devices

**Frame Acknowledgment**
- Device sends ACK when frame rendered
- Server detects packet loss
- Automatic retransmission

**Compression**
- RLE for solid color regions
- Delta encoding for animations
- Reduce bandwidth 50-90%

**Synchronization**
- Frame timestamp for coordinated playback
- Multiple devices render at exact same time
- Critical for synchronized effects

### 10.2 Version Compatibility

**Version Field:**
- Add protocol version to discovery response
- Future versions maintain backward compatibility
- Server adapts to device capabilities

## 11. Security

### 11.1 Threat Model

**Threats:**
- Unauthorized access to LED control
- Packet injection/spoofing
- Denial of service (packet flood)
- Privacy (image content visible on network)

**Current State:**
- No authentication
- No encryption
- No access control

### 11.2 Future Hardening

**Authentication:**
- Pre-shared key
- Packet signing (HMAC)

**Encryption:**
- AES for image data
- TLS over TCP (alternative transport)

**Access Control:**
- Device whitelist (MAC addresses)
- Server IP whitelist

## 12. Reference Implementation

### 12.1 Server

**Language:** Node.js
**File:** `server/gradientFrameServer.js`
**Dependencies:** dgram, express

**Features:**
- Multiple pattern generators
- Inter-packet delay control
- Discovery request transmission
- Response collection and parsing

### 12.2 Device

**Platform:** ESP8266
**Language:** C++ (Arduino framework)
**File:** `src/main.cpp`
**Libraries:** ESP8266WiFi, ArduinoJson, Adafruit_NeoPixel

**Features:**
- UDP frame reception
- Chunked frame reassembly
- Multiple mapping modes
- Discovery response generation

## 13. Testing

### 13.1 Test Tools

**test-protocol.js** - Command-line frame sender
```bash
node test-protocol.js gradient 16 16 0
node test-protocol.js solid 24 100 0 255 0 0
```

**curl** - HTTP API testing
```bash
curl -X POST http://localhost:3001/api/discover
curl http://localhost:3001/api/devices
```

**tcpdump** - Packet inspection
```bash
tcpdump -i any -n udp port 7777 -X
tcpdump -i any -n udp port 7778 -X
```

### 13.2 Test Scenarios

1. **Single device, single chunk**
   - 16x16 RGB332 frame
   - Verify reception and render

2. **Single device, multiple chunks**
   - 48x48 RGB565 frame (4 chunks)
   - Verify chunking and reassembly

3. **Multiple devices**
   - 3+ devices with different mappings
   - Verify each displays correct region

4. **Discovery**
   - Send discovery request
   - Verify all devices respond
   - Check JSON parsing

5. **Error handling**
   - Send invalid frames
   - Missing chunks
   - Timeout behavior

6. **Performance**
   - Measure frame rate
   - Monitor packet loss
   - Test network load

## 14. Troubleshooting

### 14.1 No Frames Received

- Check broadcast address matches subnet
- Verify firewall allows UDP 7777
- Check ESP serial output for errors
- Use tcpdump to verify packets sent

### 14.2 Incomplete Frames

- Increase inter-packet delay
- Check WiFi signal strength (RSSI)
- Reduce frame size
- Monitor rejectedPackets counter

### 14.3 Wrong Colors

- Verify RGB type matches (332 vs 565)
- Check byte order (little-endian)
- Test with solid color patterns
- Verify LED strip type (WS2812B vs others)

### 14.4 No Discovery Response

- Check response port open (7778)
- Verify server IP reachable from ESP
- Check ESP serial for JSON send errors
- Test with curl to server

## 15. Appendix

### 15.1 Glossary

- **Chunk** - Portion of frame that fits in one UDP packet
- **Frame** - Complete image with width×height pixels
- **Mapping** - Configuration defining which image region a device displays
- **Serpentine** - Alternating row direction in LED wiring
- **Sample Mode** - Method for extracting pixels (nearest vs interpolated)

### 15.2 Byte Order Examples

**Little-Endian (Width=1024=0x0400):**
```
Memory: [0x00, 0x04]
        LSB   MSB
```

**Big-Endian (IP=192.168.1.10=0xC0A8010A):**
```
Memory: [0xC0, 0xA8, 0x01, 0x0A]
        Byte0  Byte1 Byte2 Byte3
```

### 15.3 Packet Examples

**Discovery Request:**
```
00 01 00 00 00 00 00 00 C0 A8 01 0A 1E 62
│  │  │  │              │              └─ Port 7778 (BE)
│  │  │  └─ SubType=0   └─ IP 192.168.1.10 (BE)
│  │  └─ Chunk=0
│  └─ Counter=1
└─ Type=0
```

**Image Frame (16x16 RGB332):**
```
01 05 00 00 10 00 10 00 [256 bytes pixel data]
│  │  │  │  │     └─ Height=16 (LE)
│  │  │  │  └─ Width=16 (LE)
│  │  │  └─ RGB=0 (RGB332)
│  │  └─ Chunk=0
│  └─ Counter=5
└─ Type=1
```

---

**End of FataMorgana Protocol Specification v1.0**
