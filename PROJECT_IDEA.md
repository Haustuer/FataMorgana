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

For `FrameType = 0`, the payload contains configuration data instead of image pixels. The exact config payload format is still open and should be defined separately.

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

## Open Points For Review

These are the main points we should review together:

1. Should `RGB565` use little-endian or big-endian byte order?
2. What should the payload format of a `config frame` be?
3. Should the ESP configuration page be read-only, or should it also allow editing settings?
4. How should a 2D selected region be mapped onto a 1D LED strip?
5. Should the server send full images every time, or only changed areas?
6. Should multiple ESPs all listen to the same UDP stream, or should each device get its own stream?

## Short Goal Statement

The goal is to build a system where:

- the server broadcasts image frames over UDP,
- ESP devices listen and render their assigned section,
- and each ESP exposes a web page showing how it is configured to interpret those frames.


