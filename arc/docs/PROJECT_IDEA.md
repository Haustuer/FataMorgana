# FataMorgana Project Idea

## Overview

The core idea of this project is to have a server publish image data as UDP frames, and one or more ESP-based devices listen for those frames and display the relevant pixel data on connected LED strips.

Each ESP should:

- listen for incoming UDP frame data,
- extract the part of the frame it is responsible for,
- render that data on its LED strip,
- and provide a small web interface showing its current frame configuration.

## Basic Architecture

### Server

The server is responsible for generating or forwarding image data and publishing it as UDP frames at a steady interval.

The image does not need to be sent every single tick. Instead, a new picture or frame buffer can be published every couple of ticks, depending on the desired animation speed and network load.

### ESP Device

Each ESP listens for the UDP frames and updates its LEDs based on the incoming data.

In addition to receiving frames, the ESP should host a small website that displays its current configuration. That page should make it easy to inspect how the device interprets incoming frames.

## Frame Selection Concept

The important idea is that the full image is larger than what a single LED strip may display.

Because of that, each ESP should be configurable to select a specific region from the incoming image.

Examples:

- one ESP could display a single row from the image,
- another ESP could display a rectangular section,
- another ESP could display a square area,
- or a device could map a custom subsection of the full frame onto its LEDs.

This makes it possible to distribute one larger image across multiple physical devices.

## ESP Configuration Page

The ESP-hosted website should show the configuration used to interpret incoming frames.

Useful information for that page could include:

- device name or ID,
- current IP address,
- UDP port,
- expected frame width and height,
- selected region within the frame,
- mapping mode, such as row or square,
- LED count,
- and possibly a small preview or textual summary of the selected area.

## Possible Data Model

At a high level, each image frame could be described by:

- overall image width,
- overall image height,
- frame type,
- frame counter,
- pixel format,
- and raw pixel payload.

## UDP Frame Format

We define a compact UDP transfer protocol with a fixed 8-byte header followed by pixel data.

### Header Layout

The UDP frame starts with these bytes:

| Byte Offset | Size | Name | Description |
| --- | --- | --- | --- |
| `0` | 1 byte | `FrameType` | `0 = config frame`, `1 = image frame start`, `2 = image frame continuation` |
| `1` | 1 byte | `FrameCounter` | Logical image index or sequence counter |
| `2` | 1 byte | `ChunkIndex` | Zero-based index of this packet inside the image |
| `3` | 1 byte | `RGBType` | Pixel format type: `0 = RGB332`, `1 = RGB565` |
| `4` | 2 bytes | `width` | Full image width in pixels |
| `6` | 2 bytes | `height` | Full image height in pixels |

That means:

- bytes `0` to `7` are the header,
- byte `8` is the start of the pixel payload.

### Pixel Format Types

- `0` = `RGB332`
- `1` = `RGB565`

This allows the sender and receiver to agree on how the payload must be interpreted.

### Payload Layout

Starting at byte `8`, the packet co11ayload.

For `FrameType = 1` and `FrameType = 2`, the payload contains image pixel data.

The image payload is expected to be in row-major order:

- first all pixels of row `0` from left to right,
- then all pixels of row `1`,
- and so on until the full frame is transferred.

For `FrameType = 0`, the payload contains configuration data instead of image pixels. See Config Frame Format below.

### Payload Size

For image frames, the payload size depends on the selected pixel format:

- `RGB332`: `width * height * 1` bytes
- `RGB565`: `width * height * 2` bytes

So the full UDP packet size is:

- `8 + (width * height * 1)` bytes for `RGB332`
- `8 + (width * height * 2)` bytes for `RGB565`

### Practical UDP Size Limit

The theoretical maximum UDP payload is `65507` bytes, but for ESP devices on WiFi it is safer to stay much smaller to avoid IP fragmentation.

A practical target is a total UDP packet size of about `1200` bytes.

With an 8-byte protocol header, that leaves:

- `1200 - 8 = 1192` bytes for payload

From that, the practical image limits become:

- `RGB332`: `1192 / 1 = 1192` pixels maximum
- `RGB565`: `1192 / 2 = 596` pixels maximum

### 16:9 Example

For an exact `16:9` image, we can write:

- `width = 16k`
- `height = 9k`
- `pixels = 16k * 9k = 144k^2`

That means:

- for `RGB332`, `144k^2 <= 1192`
- for `RGB565`, `144k^2 <= 596`

In both cases, the largest exact `16:9` step that still fits is `k = 2`, which gives:

- `width = 32`
- `height = 18`
- `pixels = 576`

Packet sizes for that example:

- `RGB332`: `8 + 576 * 1 = 584` bytes
- `RGB565`: `8 + 576 * 2 = 1160` bytes

The next exact `16:9` size would be `48 x 27`, which has `1296` pixels and does not fit into the practical `1200`-byte target.

### Config Frame Format (FrameType = 0)

For config frames, byte `3` (normally `RGBType`) is repurposed as `ConfigSubType`:

| ConfigSubType | Name | Description |
| --- | --- | --- |
| `0` | Discovery | Request all devices to respond with their configuration |
| `1` | Set Mapping | Set device mapping mode (future) |
| `2` | Set Brightness | Set LED brightness (future) |
| `3` | Identify | Make a specific device flash its LEDs |

#### Discovery Frame (SubType = 0)

Header bytes 0-7 as normal, with `FrameType = 0` and `ConfigSubType = 0`.

Payload (6 bytes starting at byte 8):

| Byte Offset | Size | Name | Description |
| --- | --- | --- | --- |
| `8-11` | 4 bytes | `ServerIP` | Server IP address (big-endian, network byte order) |
| `12-13` | 2 bytes | `ResponsePort` | UDP port for device responses (big-endian) |

Devices respond via unicast UDP to the specified IP and port with a 69-byte binary response containing their configuration.

**Discovery Response Format (69 bytes):**

| Byte Range | Size | Description |
| --- | --- | --- |
| 0-3 | 4 bytes | Magic bytes "FATA" (0x46415441) |
| 4 | 1 byte | Protocol version |
| 5 | 1 byte | Response type (0x01 = discovery) |
| 6-7 | 2 bytes | Reserved |
| 8-11 | 4 bytes | Device IP address (big-endian) |
| 12-17 | 6 bytes | MAC address |
| 18-21 | 4 bytes | Chip ID (little-endian) |
| 22-25 | 4 bytes | Uptime in milliseconds (little-endian) |
| 26-27 | 2 bytes | LED count (little-endian) |
| 28 | 1 byte | LED pin number |
| 29 | 1 byte | Brightness (0-255) |
| 30 | 1 byte | Firmware major version |
| 31 | 1 byte | Firmware minor version |
| 32 | 1 byte | Mapping mode (0=row, 1=column, 2=rectangle) |
| 33 | 1 byte | Sample mode (0=pixel, 1=interpolated) |
| 34-35 | 2 bytes | Row/column index (little-endian) |
| 36-37 | 2 bytes | Line pixels count (little-endian) |
| 38-39 | 2 bytes | Rectangle X offset (little-endian) |
| 40-41 | 2 bytes | Rectangle Y offset (little-endian) |
| 42-43 | 2 bytes | Rectangle width (little-endian) |
| 44-45 | 2 bytes | Rectangle height (little-endian) |
| 46 | 1 byte | Serpentine mode (0=none, 1=horizontal, 2=vertical) |
| 47 | 1 byte | Transform byte (bits 0-1: rotation, bit 2: flipX, bit 3: flipY, bit 4: flipZ) |
| 48-51 | 4 bytes | Accepted packets count (little-endian) |
| 52-55 | 4 bytes | Rejected packets count (little-endian) |
| 56-59 | 4 bytes | Rendered frames count (little-endian) |
| 60-61 | 2 bytes | Last frame width (little-endian) |
| 62-63 | 2 bytes | Last frame height (little-endian) |
| 64-67 | 4 bytes | Gamma correction (float, little-endian) |
| 68 | 1 byte | Out-of-bounds mode (0=black, 1=clamp, 2=mirror) |

#### Identify Frame (SubType = 3)

Header bytes 0-7 as normal, with `FrameType = 0` and `ConfigSubType = 3`.

Payload (6 bytes starting at byte 8):

| Byte Offset | Size | Name | Description |
| --- | --- | --- | --- |
| `8` | 1 byte | `TargetType` | `0 = IP address`, `1 = Chip ID` |
| `9-12` | 4 bytes | `TargetValue` | If TargetType=0: IP address (big-endian). If TargetType=1: Chip ID (little-endian) |
| `13` | 1 byte | `FlashCount` | Number of times to flash LEDs (1-255) |

The targeted device will flash its LEDs the specified number of times to help identify its physical location.

**Behavior:**
- Each device checks if it matches the target (by IP or Chip ID)
- If matched, the device flashes all LEDs white for `FlashCount` times
- Each flash cycle: 200ms on (white), 200ms off (black)
- After flashing, the device restores the previous display by re-rendering the last frame
- The server sends this as a multicast packet, but only the targeted device responds visually

### Notes

- Width is stored as 2 bytes, so it can represent values up to `65535`.
- Height is stored as 2 bytes, so it can represent values up to `65535`.
- `FrameCounter` should increase for each new logical image so receivers can detect dropped or out-of-order packets.
- `ChunkIndex` tells the receiver which packet of the current image it is processing.
- `FrameType = 1` marks the first packet of an image, and `FrameType = 2` marks following packets if one image spans multiple UDP packets.
- `width` and `height` describe the full image dimensions, not just the payload chunk in a continuation packet.
- `ChunkCount` is not sent directly. It is derived from `width`, `height`, `RGBType`, and the agreed maximum payload size.
- To derive `ChunkCount`, use `ceil(totalImageBytes / maxPayloadBytesPerPacket)`.
- This only works if sender and receiver use the same maximum payload size for image packets.

Each ESP configuration could define:

- start X,
- start Y,
- selected width,
- selected height,
- mapping direction,
- and how the selected pixels are copied onto the LED strip.

## Example Use Case

The server publishes an image frame over UDP.

One ESP is configured to display row 5 of the image.

Another ESP is configured to display a 10x10 square starting at coordinate `(20, 30)`.

Each device reads the same incoming image frame, extracts only its assigned section, and renders that section onto its LEDs.

## Implemented Features

### Discovery Protocol
The server can broadcast a discovery request to find all devices on the network. Each device responds with its configuration, including:
- IP address and MAC address
- Chip ID and firmware version
- LED count and brightness
- Mapping mode and configuration
- Out-of-bounds handling mode
- Statistics (packets accepted/rejected, frames rendered)

### Identify Feature
The server can send an identify request to make a specific device flash its LEDs. This helps physically locate devices:
- Target by IP address or Chip ID
- Configurable flash count (1-255 times)
- Device flashes white then restores previous display

### Out-of-Bounds Handling
When a device requests pixels outside the transmitted frame boundaries, the system handles it according to the configured mode.

**Configuration:**
- Can be set via device web interface at `http://<device-ip>`
- Can be set programmatically via `client.setOOBMode(mode)`
- Included in discovery response (byte 68)
- Auto-saves when changed in web UI

**Black Mode (0)** - Default, safe option
- Returns black (0,0,0) for any out-of-bounds pixel
- Useful when devices should only show transmitted content
- No visual artifacts at boundaries

**Clamp Mode (1)** - Edge repeat/hold
- Uses the nearest border pixel for out-of-bounds requests
- Top edge pixels extend upward, bottom pixels extend downward
- Left edge pixels extend leftward, right edge pixels extend rightward
- Creates a "stretched edge" effect
- Useful for smooth gradients or patterns that should extend

**Mirror Mode (2)** - Boundary reflection
- Reflects/mirrors the image at boundaries
- A pixel at position -1 becomes position 1 (mirrored)
- A pixel at position width+1 becomes position width-1 (mirrored)
- Creates symmetric reflections at edges
- Useful for patterns that should tile seamlessly
- Falls back to black if mirrored position is also out-of-bounds

## Open Points For Review

These are the main points we should review together:

1. ✅ ~~Should `RGB565` use little-endian or big-endian byte order?~~ **Resolved: Uses little-endian**
2. ✅ ~~What should the payload format of a `config frame` be?~~ **Resolved: See Config Frame Format section above**
3. Should the ESP configuration page be read-only, or should it also allow editing settings?
4. ✅ ~~How should a 2D selected region be mapped onto a 1D LED strip?~~ **Resolved: Supports row, column, and rectangle modes with serpentine and transform options**
5. Should the server send full images every time, or only changed areas?
6. ✅ ~~Should multiple ESPs all listen to the same UDP stream, or should each device get its own stream?~~ **Resolved: All devices listen to the same multicast stream and extract their region**

## Server API Endpoints

The server provides a web-based control panel and REST API:

### Web Interface
- **Control Panel**: Send test patterns (gradient, solid, checkerboard, rainbow)
- **Image Upload**: Upload and send custom images
- **Device Discovery**: Find and manage devices on the network
- **Visualizer**: See the last sent frame with device regions overlaid
- **Coverage Map**: Calculate which pixels are covered by which devices

### API Endpoints
- `GET /api/config` - Get server configuration and settings
- `POST /api/frame` - Send a frame with test pattern
- `POST /api/frame/image` - Send a custom image
- `POST /api/discover` - Discover all devices on network
- `POST /api/identify` - Identify a specific device by IP or Chip ID
- `GET /api/devices` - List all discovered devices
- `GET /api/coverage` - Calculate coverage for image dimensions

## Short Goal Statement

The goal is to build a system where:

- the server broadcasts image frames over UDP,
- ESP devices listen and render their assigned section,
- each ESP exposes a web page showing how it is configured to interpret those frames,
- and the server provides a control panel to manage devices and send images.


