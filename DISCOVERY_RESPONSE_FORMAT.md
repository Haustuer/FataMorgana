# Discovery Response Binary Format

## Overview

Discovery responses are sent as **raw UDP binary packets** instead of JSON for efficiency and consistency with the FataMorgana protocol.

## Packet Structure

Total size: **64 bytes** (fixed)

```
Byte Range | Field                    | Type      | Description
-----------|--------------------------|-----------|----------------------------------
0-3        | Magic Bytes              | uint32    | 0x46415441 ("FATA" in ASCII)
4          | Protocol Version         | uint8     | Protocol version (1)
5          | Response Type            | uint8     | 0x01 = Discovery Response
6-7        | Reserved                 | uint16    | Reserved for future use (0x0000)
-----------|--------------------------|-----------|----------------------------------
8-11       | Device IP                | uint32    | IPv4 address (network byte order)
12-17      | MAC Address              | uint8[6]  | MAC address (6 bytes)
18-21      | Chip ID                  | uint32    | ESP Chip ID
22-25      | Uptime                   | uint32    | Milliseconds since boot
-----------|--------------------------|-----------|----------------------------------
26-27      | LED Count                | uint16    | Total number of LEDs
28         | LED Pin                  | uint8     | GPIO pin number
29         | Brightness               | uint8     | Current brightness (0-255)
30         | Firmware Major           | uint8     | Firmware version major
31         | Firmware Minor           | uint8     | Firmware version minor
-----------|--------------------------|-----------|----------------------------------
32         | Mapping Mode             | uint8     | 0=Row, 1=Column, 2=Rectangle
33         | Sample Mode              | uint8     | 0=Pixel, 1=Interpolated
34-35      | Row/Column Index         | uint16    | Row or Column index
36-37      | Line Pixels              | uint16    | LEDs for row/column mode
38-39      | Rect X                   | uint16    | Rectangle X offset
40-41      | Rect Y                   | uint16    | Rectangle Y offset
42-43      | Rect Width               | uint16    | Rectangle width
44-45      | Rect Height              | uint16    | Rectangle height
46         | Serpentine               | uint8     | 0=No, 1=Yes
47         | Transform                | uint8     | Rotation and flip flags (see below)
-----------|--------------------------|-----------|----------------------------------
48-51      | Accepted Packets         | uint32    | Total accepted packets
52-55      | Rejected Packets         | uint32    | Total rejected packets
56-59      | Rendered Frames          | uint32    | Total rendered frames
60-61      | Last Frame Width         | uint16    | Last frame width
62-63      | Last Frame Height        | uint16    | Last frame height
```

## Field Details

### Magic Bytes (0-3)
- **Value:** `0x46415441` (ASCII: "FATA")
- **Purpose:** Identify FataMorgana discovery responses
- **Byte order:** Big-endian

### Protocol Version (4)
- **Value:** `1`
- **Purpose:** Protocol version for compatibility

### Response Type (5)
- **Value:** `0x01` for discovery response
- **Purpose:** Allow for future response types

### Device IP (8-11)
- **Format:** Network byte order (big-endian)
- **Example:** `192.168.1.100` = `0xC0A80164`

### MAC Address (12-17)
- **Format:** 6 raw bytes
- **Example:** `AA:BB:CC:DD:EE:FF` = `0xAA 0xBB 0xCC 0xDD 0xEE 0xFF`

### Chip ID (18-21)
- **Format:** 32-bit unsigned integer
- **Purpose:** Unique ESP chip identifier

### Uptime (22-25)
- **Format:** Milliseconds as uint32
- **Purpose:** Device uptime for troubleshooting

### LED Count (26-27)
- **Format:** 16-bit unsigned integer
- **Range:** 0-65535

### Mapping Mode (32)
- `0` = ROW mode
- `1` = COLUMN mode
- `2` = RECTANGLE mode

### Sample Mode (33)
- `0` = PIXEL (nearest neighbor)
- `1` = INTERPOLATED (linear interpolation)

### Serpentine (46)
- `0` = No serpentine (SERPENTINE_NONE)
- `1` = Horizontal serpentine (SERPENTINE_HORIZONTAL - zigzag left-right)
- `2` = Vertical serpentine (SERPENTINE_VERTICAL - zigzag up-down)

### Transform (47)
Byte layout for rotation and flip transforms:

```
Bit Layout:
  Bits 0-1: Rotation (0=0°, 1=90°, 2=180°, 3=270°)
  Bit 2:    Flip X (horizontal flip - mirror across vertical axis)
  Bit 3:    Flip Y (vertical flip - mirror across horizontal axis)
  Bit 4:    Flip Z (diagonal flip - transpose/swap X and Y)
  Bits 5-7: Reserved (future use)
```

**Encoding:**
```c
uint8_t transform = (rotation & 0x03) |
                    (flipX ? 0x04 : 0) |
                    (flipY ? 0x08 : 0) |
                    (flipZ ? 0x10 : 0);
```

**Decoding:**
```c
uint8_t rotation = transform & 0x03;
bool flipX = (transform & 0x04) != 0;
bool flipY = (transform & 0x08) != 0;
bool flipZ = (transform & 0x10) != 0;
```

**Examples:**
- `0x00` = No rotation, no flips
- `0x01` = 90° rotation
- `0x04` = Flip X (horizontal mirror)
- `0x0C` = Flip X + Flip Y (180° flip via mirrors)
- `0x12` = 180° rotation + Flip Z

**Transform Order:** Rotation is applied first, then flips.

## Example Packet

Device: `192.168.1.100`, MAC: `AA:BB:CC:DD:EE:FF`, 256 LEDs, Rectangle (0,0) 16×16

```
Hex dump:
46 41 54 41 01 01 00 00    Magic "FATA", Ver 1, Type 1, Reserved
C0 A8 01 64 AA BB CC DD    IP: 192.168.1.100, MAC: AA:BB:CC:DD:EE:FF
EE FF 12 34 AB CD 00 00    MAC cont., ChipID: 0x1234ABCD, Uptime: 0x00000E10
0E 10 01 00 0C 50 01 00    Uptime cont., LEDs: 256, Pin: 12, Brightness: 80
01 00 02 00 00 00 00 00    Firmware: 1.0, Mode: 2 (Rect), Sample: 0
10 00 00 00 00 00 10 00    RowIdx: 0, LinePixels: 0, RectX: 0, RectY: 0
10 00 00 00 00 00 00 00    RectW: 16, RectH: 16, Serpentine: 0, Reserved
00 05 F3 00 00 00 0C 00    AcceptedPkts: 1523, RejectedPkts: 12
00 01 FB 00 18 00 64 00    RenderedFrames: 507, LastW: 24, LastH: 100
```

## Advantages

1. **Fixed Size:** Always 64 bytes, easy to parse
2. **Efficient:** No JSON parsing overhead
3. **Fast:** Binary is faster to serialize/deserialize
4. **Bandwidth:** Smaller than JSON (~300 bytes → 64 bytes)
5. **Consistent:** Matches binary protocol philosophy
6. **Validated:** Magic bytes prevent false positives

## Server Parsing (Node.js)

```javascript
function parseDiscoveryResponse(buffer) {
  if (buffer.length !== 64) {
    throw new Error('Invalid discovery response length');
  }

  // Check magic bytes
  const magic = buffer.readUInt32BE(0);
  if (magic !== 0x46415441) { // "FATA"
    throw new Error('Invalid magic bytes');
  }

  const version = buffer.readUInt8(4);
  const type = buffer.readUInt8(5);

  if (type !== 0x01) {
    throw new Error('Invalid response type');
  }

  // Parse IP address (network byte order)
  const ip = `${buffer.readUInt8(8)}.${buffer.readUInt8(9)}.${buffer.readUInt8(10)}.${buffer.readUInt8(11)}`;

  // Parse MAC address
  const mac = Array.from(buffer.subarray(12, 18))
    .map(b => b.toString(16).padStart(2, '0').toUpperCase())
    .join(':');

  const chipId = buffer.readUInt32LE(18);
  const uptime = buffer.readUInt32LE(22);

  const ledCount = buffer.readUInt16LE(26);
  const ledPin = buffer.readUInt8(28);
  const brightness = buffer.readUInt8(29);
  const firmwareMajor = buffer.readUInt8(30);
  const firmwareMinor = buffer.readUInt8(31);

  const mappingMode = buffer.readUInt8(32);
  const sampleMode = buffer.readUInt8(33);
  const rowIndex = buffer.readUInt16LE(34);
  const columnIndex = rowIndex; // Same field
  const linePixels = buffer.readUInt16LE(36);
  const rectX = buffer.readUInt16LE(38);
  const rectY = buffer.readUInt16LE(40);
  const rectWidth = buffer.readUInt16LE(42);
  const rectHeight = buffer.readUInt16LE(44);
  const serpentine = buffer.readUInt8(46);  // 0=none, 1=horizontal, 2=vertical

  const acceptedPackets = buffer.readUInt32LE(48);
  const rejectedPackets = buffer.readUInt32LE(52);
  const renderedFrames = buffer.readUInt32LE(56);
  const lastFrameWidth = buffer.readUInt16LE(60);
  const lastFrameHeight = buffer.readUInt16LE(62);

  const modeNames = ['row', 'column', 'rectangle'];
  const sampleModeNames = ['pixel', 'interpolated'];
  const serpentineModeNames = ['none', 'horizontal', 'vertical'];

  return {
    protocol: 'FataMorgana',
    version,
    type: 'discovery_response',
    device: {
      ip,
      mac,
      chipId: chipId.toString(16).toUpperCase(),
      hostname: `ESP-${chipId.toString(16).substring(0, 6).toUpperCase()}`,
      firmware: `${firmwareMajor}.${firmwareMinor}`,
      uptime
    },
    hardware: {
      ledCount,
      ledPin,
      brightness
    },
    mapping: {
      mode: mappingMode,
      modeName: modeNames[mappingMode] || 'unknown',
      sampleMode,
      sampleModeName: sampleModeNames[sampleMode] || 'unknown',
      rowIndex,
      columnIndex,
      linePixels,
      rectX,
      rectY,
      rectWidth,
      rectHeight,
      serpentine,
      serpentineModeName: serpentineModeNames[serpentine] || 'unknown'
    },
    status: {
      acceptedPackets,
      rejectedPackets,
      renderedFrames,
      lastFrameWidth,
      lastFrameHeight
    }
  };
}
```

## ESP Implementation (C++)

```cpp
void sendBinaryDiscoveryResponse(IPAddress serverIP, uint16_t serverPort) {
  uint8_t response[64];
  memset(response, 0, sizeof(response));

  // Magic bytes "FATA" (0x46415441)
  response[0] = 0x46; // 'F'
  response[1] = 0x41; // 'A'
  response[2] = 0x54; // 'T'
  response[3] = 0x41; // 'A'

  // Protocol version and type
  response[4] = 1;    // Version
  response[5] = 0x01; // Discovery response type

  // Device IP (network byte order)
  IPAddress localIP = WiFi.localIP();
  response[8] = localIP[0];
  response[9] = localIP[1];
  response[10] = localIP[2];
  response[11] = localIP[3];

  // MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(response + 12, mac, 6);

  // Chip ID (little-endian)
  uint32_t chipId = ESP.getChipId();
  response[18] = chipId & 0xFF;
  response[19] = (chipId >> 8) & 0xFF;
  response[20] = (chipId >> 16) & 0xFF;
  response[21] = (chipId >> 24) & 0xFF;

  // Uptime (little-endian)
  uint32_t uptime = millis();
  response[22] = uptime & 0xFF;
  response[23] = (uptime >> 8) & 0xFF;
  response[24] = (uptime >> 16) & 0xFF;
  response[25] = (uptime >> 24) & 0xFF;

  // LED info
  response[26] = LED_COUNT & 0xFF;
  response[27] = (LED_COUNT >> 8) & 0xFF;
  response[28] = LED_PIN;
  response[29] = strip.getBrightness();
  response[30] = 1; // Firmware major
  response[31] = 0; // Firmware minor

  // Mapping config
  response[32] = mappingConfig.mode;
  response[33] = mappingConfig.sampleMode;
  response[34] = mappingConfig.rowIndex & 0xFF;
  response[35] = (mappingConfig.rowIndex >> 8) & 0xFF;
  response[36] = mappingConfig.linePixels & 0xFF;
  response[37] = (mappingConfig.linePixels >> 8) & 0xFF;
  response[38] = mappingConfig.rectX & 0xFF;
  response[39] = (mappingConfig.rectX >> 8) & 0xFF;
  response[40] = mappingConfig.rectY & 0xFF;
  response[41] = (mappingConfig.rectY >> 8) & 0xFF;
  response[42] = mappingConfig.rectWidth & 0xFF;
  response[43] = (mappingConfig.rectWidth >> 8) & 0xFF;
  response[44] = mappingConfig.rectHeight & 0xFF;
  response[45] = (mappingConfig.rectHeight >> 8) & 0xFF;
  response[46] = mappingConfig.serpentine;  // 0=none, 1=horizontal, 2=vertical

  // Statistics (little-endian)
  response[48] = acceptedPackets & 0xFF;
  response[49] = (acceptedPackets >> 8) & 0xFF;
  response[50] = (acceptedPackets >> 16) & 0xFF;
  response[51] = (acceptedPackets >> 24) & 0xFF;

  response[52] = rejectedPackets & 0xFF;
  response[53] = (rejectedPackets >> 8) & 0xFF;
  response[54] = (rejectedPackets >> 16) & 0xFF;
  response[55] = (rejectedPackets >> 24) & 0xFF;

  response[56] = renderedFrames & 0xFF;
  response[57] = (renderedFrames >> 8) & 0xFF;
  response[58] = (renderedFrames >> 16) & 0xFF;
  response[59] = (renderedFrames >> 24) & 0xFF;

  response[60] = lastSeenWidth & 0xFF;
  response[61] = (lastSeenWidth >> 8) & 0xFF;
  response[62] = lastSeenHeight & 0xFF;
  response[63] = (lastSeenHeight >> 8) & 0xFF;

  // Send UDP packet
  udp.beginPacket(serverIP, serverPort);
  udp.write(response, sizeof(response));
  udp.endPacket();
}
```

## Byte Order Summary

- **IP Address:** Big-endian (network byte order) - bytes 8-11
- **All other multi-byte fields:** Little-endian (for ESP efficiency)
- **Magic bytes:** Big-endian (ASCII "FATA")

## Migration Notes

### Old JSON Format (300+ bytes)
```json
{
  "protocol": "FataMorgana",
  "version": 1,
  "device": { ... },
  "hardware": { ... },
  "mapping": { ... },
  "status": { ... }
}
```

### New Binary Format (64 bytes)
- 78% size reduction
- Faster parsing on ESP8266
- No ArduinoJson library dependency
- Fixed-size simplifies buffer management

## Testing

```bash
# Send discovery request
curl -X POST http://localhost:3001/api/discover

# Monitor UDP responses with hexdump
sudo tcpdump -i any 'udp port 7778' -X

# Should see 64-byte packets starting with: 46 41 54 41 01 01 ...
```
