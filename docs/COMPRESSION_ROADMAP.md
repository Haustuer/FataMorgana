# FataMorgana Compression Roadmap

**Goal:** Enable 100×100 source image transmission to support scaling from 30 to 100+ ESP8266 devices

**Date:** 2026-06-04
**Status:** Planning Phase

---

## Executive Summary

Current FataMorgana protocol supports up to 69×69 pixels (4,800 byte limit). To support immersive installations with 50-100 devices displaying 100×100 source images, we need compression.

**Recommended Solution:** MJPEG (Motion JPEG) compression
- Compresses 100×100 images from 10,000-20,000 bytes → 1,500-3,000 bytes
- Compatible with ESP8266 (software decode)
- Scales to ESP32 (hardware decode) for future upgrades
- Handles any content type (photos, video, patterns, text)

---

## Current State

### Hardware
- **Deployed:** 30 ESP8266 devices
- **LED Configuration:** 100 LEDs per device (vertical strips/columns)
- **Physical Display:** 30×100 pixels (3,000 LEDs total)
- **Platform:** ESP8266 D1 Mini Lite (80MHz, ~50KB free RAM)

### Protocol Limitations
- **Max frame size:** 4,800 bytes
- **RGB332 max:** 69×69 pixels (4,761 bytes)
- **RGB565 max:** 48×48 pixels (4,608 bytes)
- **Transport:** UDP multicast (239.255.42.1:7777)
- **Chunking:** 8-byte header + 1,192 byte payload per packet

### Current Performance
- **Frame rate:** 30 fps @ 48×48 RGB565
- **Bandwidth:** ~600 Kbps per device
- **Latency:** <50ms frame assembly time

---

## Future Vision

### Expansion Plan
- **Phase 1:** 50 devices → 50×100 display (5,000 LEDs)
- **Phase 2:** 100 devices → 100×100 display (10,000 LEDs)
- **Phase 3:** 160 devices → 160×100 display (16,000 LEDs)

### Target Capabilities
- **Source Resolution:** 100×100 pixels (expandable to 160×100+)
- **Content Types:** Video streams, animations, static images, live content
- **Frame Rate:** 8-10 fps (ESP8266) / 20-30 fps (ESP32)
- **Quality:** High-quality JPEG encoding (configurable 60-90 quality)

---

## Technical Analysis

### Data Size Challenge

| Resolution | RGB332 | RGB565 | Protocol Limit | Compression Needed |
|------------|--------|--------|----------------|-------------------|
| 48×48      | 2,304  | 4,608  | 4,800         | None ✅           |
| 69×69      | 4,761  | 9,522  | 4,800         | 2× (RGB565 only)  |
| 100×100    | 10,000 | 20,000 | 4,800         | 2-4×             |
| 160×100    | 16,000 | 32,000 | 4,800         | 3-7×             |

### Compression Options Evaluated

#### 1. RLE (Run-Length Encoding)
- **Compression:** 2-10× (pattern-dependent)
- **Decode Time:** 5-10ms
- **RAM Usage:** Minimal
- **Pros:** Simple, fast, low memory
- **Cons:** Poor compression on photos/video
- **Best For:** UI elements, text, simple patterns

#### 2. MJPEG (Motion JPEG) ⭐ **SELECTED**
- **Compression:** 5-15× (typical 8-10×)
- **Decode Time:** 100-150ms (ESP8266 software), 20-50ms (ESP32 hardware)
- **RAM Usage:** ~30KB decode buffer
- **Pros:** Universal, handles any content, standard format
- **Cons:** Slower decode on ESP8266, higher memory usage
- **Best For:** Photos, video, general content

#### 3. H.264 Video Codec
- **Compression:** 50-100×
- **Decode Time:** N/A (requires hardware decoder)
- **RAM Usage:** 5-10MB minimum
- **Pros:** Best compression
- **Cons:** Not feasible on ESP8266, minimal on ESP32
- **Verdict:** Not suitable for current hardware

#### 4. QOI (Quite OK Image)
- **Compression:** 2-5×
- **Decode Time:** 5-10ms
- **RAM Usage:** Low
- **Pros:** Simple, fast
- **Cons:** Moderate compression
- **Best For:** Alternative to RLE

### Selected: MJPEG Compression

**100×100 JPEG Performance:**
```
Raw RGB888:    30,000 bytes
Raw RGB565:    20,000 bytes
Raw RGB332:    10,000 bytes

JPEG Quality 90: ~3,000 bytes (10:1)
JPEG Quality 80: ~2,000 bytes (15:1) ← Target
JPEG Quality 70: ~1,500 bytes (20:1)
```

**ESP8266 Feasibility:**
- ✅ TJpgDec library available (software decoder)
- ✅ 2,000 bytes JPEG fits in 2 UDP packets
- ✅ 30KB decode buffer fits in available RAM (~50KB free)
- ⚠️ 100-150ms decode time limits to ~8-10 fps
- ✅ Good enough for most installations

**ESP32 Future Path:**
- ✅ Hardware JPEG decoder built-in
- ✅ 20-50ms decode time → 20-30 fps
- ✅ Simple firmware upgrade path

---

## Implementation Plan

### Phase 1: Core JPEG Support (5 days)

#### Day 1: Protocol Extension
**File:** `lib/FataMorgana/src/FataMorganaProtocol.h`

```cpp
// Add new frame type
constexpr uint8_t FATAMORGANA_FRAME_TYPE_IMAGE_JPEG = 5;

// JPEG quality levels (for future config frames)
constexpr uint8_t FATAMORGANA_JPEG_QUALITY_HIGH = 90;
constexpr uint8_t FATAMORGANA_JPEG_QUALITY_MEDIUM = 80;
constexpr uint8_t FATAMORGANA_JPEG_QUALITY_LOW = 70;
```

**File:** `platformio.ini`

```ini
lib_deps =
    FataMorgana
    FataMorgana-WebUI
    tzapu/WiFiManager@^2.0.17
    bodmer/TJpg_Decoder@^1.0.9  # Add JPEG decoder
```

#### Day 2-3: Server-Side JPEG Encoding
**File:** `server/jpegFrameSender.js` (new)

```javascript
const sharp = require('sharp');
const dgram = require('dgram');

class JpegFrameSender {
  /**
   * Convert and compress image to JPEG, send via multicast
   * @param {Buffer} rgbBuffer - Raw RGB888 data
   * @param {number} width - Image width
   * @param {number} height - Image height
   * @param {number} quality - JPEG quality (60-95)
   */
  async sendJpegFrame(rgbBuffer, width, height, quality = 80) {
    // Compress to JPEG
    const jpegBuffer = await sharp(rgbBuffer, {
      raw: { width, height, channels: 3 }
    })
    .jpeg({ quality, chromaSubsampling: '4:4:4' })
    .toBuffer();

    console.log(`JPEG compression: ${rgbBuffer.length} → ${jpegBuffer.length} bytes (${(jpegBuffer.length / rgbBuffer.length * 100).toFixed(1)}%)`);

    // Send via existing protocol
    await this.sendChunkedFrame(jpegBuffer, width, height, FRAME_TYPE_IMAGE_JPEG);
  }

  /**
   * Send frame split into UDP packets
   */
  async sendChunkedFrame(frameData, width, height, frameType) {
    const frameBytes = frameData.length;
    const chunkCount = Math.ceil(frameBytes / MAX_PAYLOAD_SIZE);
    const frameCounter = this.frameCounter++ % 256;

    for (let chunkIdx = 0; chunkIdx < chunkCount; chunkIdx++) {
      const isFirstChunk = chunkIdx === 0;
      const packetFrameType = isFirstChunk ? frameType : FRAME_TYPE_IMAGE_CONTINUATION;

      // Build packet: [type][counter][format][chunk][width][height][reserved][reserved][payload...]
      const payloadStart = chunkIdx * MAX_PAYLOAD_SIZE;
      const payloadEnd = Math.min(payloadStart + MAX_PAYLOAD_SIZE, frameBytes);
      const payload = frameData.slice(payloadStart, payloadEnd);

      const packet = Buffer.alloc(HEADER_SIZE + payload.length);
      packet[0] = packetFrameType;
      packet[1] = frameCounter;
      packet[2] = 0xFF; // JPEG format marker (not RGB332/565)
      packet[3] = chunkIdx;
      packet.writeUInt16LE(width, 4);
      packet.writeUInt16LE(height, 6);
      payload.copy(packet, HEADER_SIZE);

      await this.sendPacket(packet);

      // Inter-packet delay
      if (INTER_PACKET_DELAY_MS > 0 && chunkIdx < chunkCount - 1) {
        await this.sleep(INTER_PACKET_DELAY_MS);
      }
    }
  }
}

module.exports = JpegFrameSender;
```

**File:** `server/gradientFrameServer.js` (modify)

```javascript
const JpegFrameSender = require('./jpegFrameSender');

// Add JPEG mode option
app.post('/api/frame/jpeg', async (req, res) => {
  const { width, height, quality, pattern } = req.body;

  // Generate or receive RGB image
  const rgbBuffer = generatePattern(pattern, width, height);

  // Send as JPEG
  const sender = new JpegFrameSender();
  await sender.sendJpegFrame(rgbBuffer, width, height, quality);

  res.json({ ok: true, originalBytes: rgbBuffer.length });
});
```

#### Day 4-5: ESP8266 JPEG Decoding
**File:** `lib/FataMorgana/src/FataMorganaClient.h`

```cpp
#include <TJpgDec.h>

class FataMorganaClient {
private:
    // JPEG decode state
    TJpgDec _jpegDecoder;
    uint8_t* _jpegDecodeBuffer;
    size_t _jpegDecodeBufferSize;
    uint16_t _jpegWidth;
    uint16_t _jpegHeight;
    bool _jpegDecodeActive;

    // JPEG decode callback (static for TJpgDec)
    static bool jpegOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

    void handleJpegFrame(const uint8_t* jpegData, size_t jpegLen, uint16_t width, uint16_t height);
    bool decodeJpeg(const uint8_t* jpegData, size_t jpegLen);
};
```

**File:** `lib/FataMorgana/src/FataMorganaClient.cpp`

```cpp
// JPEG decode callback - receives decoded RGB565 pixels
bool FataMorganaClient::jpegOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // Get client instance (stored in TJpgDec user data)
    FataMorganaClient* client = (FataMorganaClient*)TJpgDec.getUserData();
    if (!client || !client->_jpegDecodeActive) return false;

    // Copy decoded pixels to buffer
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            uint16_t px = y + row;
            uint16_t py = x + col;
            if (px < client->_jpegHeight && py < client->_jpegWidth) {
                uint16_t idx = (px * client->_jpegWidth + py) * 3;
                uint16_t rgb565 = bitmap[row * w + col];

                // Convert RGB565 → RGB888
                client->_jpegDecodeBuffer[idx]     = ((rgb565 >> 11) & 0x1F) << 3; // R
                client->_jpegDecodeBuffer[idx + 1] = ((rgb565 >> 5) & 0x3F) << 2;  // G
                client->_jpegDecodeBuffer[idx + 2] = (rgb565 & 0x1F) << 3;         // B
            }
        }
    }
    return true;
}

void FataMorganaClient::handleJpegFrame(const uint8_t* jpegData, size_t jpegLen, uint16_t width, uint16_t height) {
    unsigned long decodeStart = millis();

    // Allocate decode buffer if needed
    size_t requiredSize = width * height * 3; // RGB888
    if (!_jpegDecodeBuffer || _jpegDecodeBufferSize < requiredSize) {
        if (_jpegDecodeBuffer) free(_jpegDecodeBuffer);
        _jpegDecodeBuffer = (uint8_t*)malloc(requiredSize);
        _jpegDecodeBufferSize = requiredSize;

        if (!_jpegDecodeBuffer) {
            Serial.println(F("ERROR: Failed to allocate JPEG decode buffer"));
            return;
        }
    }

    // Setup TJpgDec
    _jpegWidth = width;
    _jpegHeight = height;
    _jpegDecodeActive = true;

    TJpgDec.setUserData(this);
    TJpgDec.setCallback(jpegOutputCallback);
    TJpgDec.setJpgScale(1); // No scaling

    // Decode JPEG
    JRESULT result = TJpgDec.drawJpg(0, 0, jpegData, jpegLen);
    _jpegDecodeActive = false;

    if (result != JDR_OK) {
        Serial.printf("ERROR: JPEG decode failed (code %d)\n", result);
        return;
    }

    unsigned long decodeTime = millis() - decodeStart;
    Serial.printf("JPEG decoded: %ux%u in %lu ms\n", width, height, decodeTime);

    // Now render from decoded buffer
    _renderer.render(_jpegDecodeBuffer, width, height,
                     FATAMORGANA_RGB888, _mapping, _strip);

    _renderedFrames++;
}

// Modify handlePacket to detect JPEG frames
void FataMorganaClient::handlePacket(const uint8_t* data, size_t length) {
    if (length < HEADER_SIZE) return;

    uint8_t frameType = data[0];
    uint8_t frameCounter = data[1];
    uint8_t rgbType = data[2];
    uint8_t chunkIndex = data[3];
    uint16_t width = data[4] | (data[5] << 8);
    uint16_t height = data[6] | (data[7] << 8);

    // Check for JPEG frame
    if (frameType == FATAMORGANA_FRAME_TYPE_IMAGE_JPEG) {
        // Start JPEG frame assembly
        if (!beginFrame(frameCounter, 0xFF, width, height)) return;
        // ... rest of chunking logic

        // When complete:
        if (_receivedChunkCount == _expectedChunkCount) {
            handleJpegFrame(_frameBuffer, _activeFrameBytes, width, height);
        }
    }
    // ... existing RGB332/RGB565 handling
}
```

**File:** `lib/FataMorgana/src/FataMorganaRenderer.h`

```cpp
// Add RGB888 support
constexpr uint8_t FATAMORGANA_RGB888 = 2;
```

---

### Phase 2: Video Streaming Integration (3 days)

#### Day 6: FFmpeg Video Pipeline
**File:** `server/videoStreamer.js` (modify existing or new)

```javascript
const { spawn } = require('child_process');
const JpegFrameSender = require('./jpegFrameSender');

class VideoStreamer {
  constructor(options) {
    this.source = options.source; // file, rtmp, webcam, screen
    this.outputWidth = options.width || 100;
    this.outputHeight = options.height || 100;
    this.fps = options.fps || 10;
    this.jpegQuality = options.quality || 80;
    this.sender = new JpegFrameSender();
  }

  start() {
    // Build FFmpeg command
    const ffmpegArgs = [
      '-i', this.source,
      '-vf', `scale=${this.outputWidth}:${this.outputHeight}:flags=lanczos`,
      '-r', this.fps.toString(),
      '-f', 'image2pipe',
      '-vcodec', 'mjpeg',
      '-q:v', this.jpegQuality.toString(),
      '-'
    ];

    this.ffmpeg = spawn('ffmpeg', ffmpegArgs);

    // Parse MJPEG stream
    this.ffmpeg.stdout.on('data', (data) => {
      this.handleMjpegData(data);
    });
  }

  handleMjpegData(data) {
    // FFmpeg outputs MJPEG stream (series of JPEG images)
    // Parse JPEG markers and extract individual frames
    // Send each frame via multicast

    // JPEG frame starts with 0xFF 0xD8, ends with 0xFF 0xD9
    // ... parsing logic ...

    this.sender.sendJpegFrame(jpegBuffer, this.outputWidth, this.outputHeight, this.jpegQuality);
  }
}

// CLI usage
const streamer = new VideoStreamer({
  source: process.argv[2], // video.mp4, rtmp://..., etc.
  width: 100,
  height: 100,
  fps: 10,
  quality: 80
});

streamer.start();
```

#### Day 7-8: Testing & Optimization

**Test Plan:**
1. Static images (various sizes, 50×50 to 100×100)
2. Generated patterns (gradient, checkerboard)
3. Video files (short clips, various resolutions)
4. RTMP streams (OBS output)
5. Multi-device testing (5, 10, 30 devices)

**Optimization targets:**
- Reduce decode time with memory tuning
- Add quality fallback for memory errors
- Implement frame skipping for stability
- Add stats/monitoring for decode performance

---

### Phase 3: Hybrid & Advanced Features (5 days)

#### Day 9-10: Adaptive Compression
**File:** `server/adaptiveCompression.js`

```javascript
class AdaptiveFrameSender {
  async sendFrame(imageBuffer, width, height) {
    // Analyze image complexity
    const complexity = await this.estimateComplexity(imageBuffer);

    // Try compression methods
    const jpeg = await this.compressJpeg(imageBuffer, width, height, 80);
    const rle = await this.compressRLE(imageBuffer, width, height);

    // Pick best method
    if (jpeg.length < rle.length && jpeg.length < 4000) {
      this.sendJpeg(jpeg, width, height);
    } else if (rle.length < 4000) {
      this.sendRLE(rle, width, height);
    } else {
      // Fall back to server-side downsample
      const downsampled = await this.downsample(imageBuffer, width, height);
      this.sendRaw(downsampled);
    }
  }

  async estimateComplexity(imageBuffer) {
    // Use sharp to calculate image statistics
    const stats = await sharp(imageBuffer).stats();
    // High entropy = complex (use JPEG)
    // Low entropy = simple (use RLE)
    return stats.entropy;
  }
}
```

#### Day 11-12: RLE Fallback Implementation
Add RLE compression as fallback for simple patterns:
- Faster decode (~5ms vs 150ms)
- Better for UI elements, text, solid colors
- Complement to JPEG

#### Day 13: Performance Tuning
- Memory pooling for decode buffers
- Frame queue management
- Error recovery and graceful degradation
- Statistics and monitoring

---

## Testing Strategy

### Unit Tests

**Server-Side:**
```javascript
// Test JPEG compression ratios
test('100x100 gradient compresses to <3KB', async () => {
  const gradient = generateGradient(100, 100);
  const jpeg = await compressJpeg(gradient, 80);
  expect(jpeg.length).toBeLessThan(3000);
});

// Test chunking
test('JPEG frame splits into correct packets', () => {
  const jpegData = Buffer.alloc(2500);
  const packets = chunkFrame(jpegData, 100, 100);
  expect(packets.length).toBe(3); // 2500 bytes / 1192 per packet
});
```

**ESP8266:**
```cpp
void testJpegDecode() {
  // Load test JPEG from SPIFFS
  File f = SPIFFS.open("/test100x100.jpg", "r");
  size_t jpegSize = f.size();
  uint8_t* jpegData = (uint8_t*)malloc(jpegSize);
  f.readBytes((char*)jpegData, jpegSize);
  f.close();

  // Decode
  unsigned long start = millis();
  bool success = decodeJpeg(jpegData, jpegSize);
  unsigned long elapsed = millis() - start;

  Serial.printf("Decode: %s, Time: %lu ms\n", success ? "OK" : "FAIL", elapsed);

  free(jpegData);
}
```

### Integration Tests

**Multi-Device Test:**
1. Set up 5-10 ESP8266 devices
2. Send 100×100 JPEG frames at 10 fps
3. Verify all devices render correctly
4. Monitor for packet loss, decode errors
5. Measure frame rate consistency

**Load Test:**
1. Simulate 100 devices (virtual or physical)
2. Send frames continuously for 1 hour
3. Monitor memory usage, decode times
4. Check for memory leaks, crashes
5. Validate frame sync across devices

### Performance Benchmarks

| Metric | Target | Acceptable | Critical |
|--------|--------|------------|----------|
| JPEG decode time (100×100) | <100ms | <150ms | <200ms |
| Memory usage | <40KB | <45KB | <50KB |
| Frame rate | 10 fps | 8 fps | 5 fps |
| Packet loss | 0% | <1% | <5% |
| Frame assembly time | <20ms | <50ms | <100ms |

---

## Migration Guide

### Updating Existing 30 Devices

**Firmware Update Process:**
1. Build new firmware with JPEG support
2. Test on 1-2 devices first
3. Roll out to remaining devices in batches
4. Keep old firmware as fallback

**Backward Compatibility:**
```cpp
// ESP firmware detects frame type automatically
void handlePacket(const uint8_t* data, size_t length) {
  uint8_t frameType = data[0];

  if (frameType == FRAME_TYPE_IMAGE_JPEG) {
    handleJpegFrame(data, length); // New
  } else {
    handleRawFrame(data, length);   // Existing
  }
}
```

**Server Compatibility:**
```javascript
// Server can send both formats
if (deviceSupportsJpeg(deviceId)) {
  sendJpegFrame(image, 100, 100);
} else {
  sendRawFrame(image, 48, 48); // Old protocol
}
```

### OTA Update Strategy

**Phase 1:** Test devices (3 devices)
- Update manually via USB
- Test for 24 hours

**Phase 2:** Pilot rollout (10 devices)
- Use OTA update
- Monitor for issues
- Roll back if problems detected

**Phase 3:** Full deployment (remaining 17 devices)
- Batch OTA updates
- Stagger updates over several hours
- Keep monitoring active

**Rollback Plan:**
- Keep old firmware binary accessible
- Script to mass-reflash if needed
- Test rollback procedure in Phase 1

---

## ESP32 Migration Path

### When to Upgrade

**Keep ESP8266 if:**
- 8-10 fps is acceptable
- Budget constrained
- Installation works well

**Upgrade to ESP32 if:**
- Need 20-30 fps
- Playing video content
- Future-proofing investment

### Firmware Portability

```cpp
// Shared code works on both platforms
#ifdef ESP32
  #include "esp_jpg_decode.h"
  #define USE_HARDWARE_JPEG 1
#else
  #include <TJpgDec.h>
  #define USE_HARDWARE_JPEG 0
#endif

void decodeJpeg(uint8_t* jpegData, size_t jpegLen) {
  #if USE_HARDWARE_JPEG
    // ESP32 hardware decoder
    esp_jpg_decode(jpegLen, JPG_SCALE_NONE, jpegData, jpegLen, &rgb_buf, &rgb_len);
  #else
    // ESP8266 software decoder
    TJpgDec.drawJpg(0, 0, jpegData, jpegLen);
  #endif
}
```

### Cost Analysis

| Component | ESP8266 | ESP32 | Difference |
|-----------|---------|-------|------------|
| Module | $2-3 | $3-5 | +$1-2 |
| Performance | 8-10 fps | 20-30 fps | 3× faster |
| RAM | 50KB | 320KB | 6× more |
| Decode | Software | Hardware | 5-7× faster |

**30 device upgrade:** $30-60 total investment for 3× performance

---

## Risk Management

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| ESP8266 RAM exhaustion | Medium | High | Add memory checks, fallback to lower resolution |
| JPEG decode too slow | Medium | Medium | Implement frame skipping, reduce quality |
| Packet loss at scale | Low | Medium | Add frame acknowledgment (optional) |
| TJpgDec compatibility | Low | High | Test extensively, have backup decoder |

### Operational Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Firmware update failure | Low | High | OTA with rollback, staged deployment |
| Performance regression | Medium | Medium | Benchmark before/after, keep old firmware |
| Device synchronization | Medium | Low | Add frame counter checks |
| Network congestion | Low | Medium | Monitor bandwidth, adjust quality |

---

## Success Criteria

### Phase 1 Complete When:
- ✅ Server can send 100×100 JPEG frames
- ✅ ESP8266 can decode and display correctly
- ✅ Frame rate ≥8 fps sustained
- ✅ Memory usage <45KB
- ✅ No crashes after 1 hour continuous operation
- ✅ Works with 30 devices simultaneously

### Phase 2 Complete When:
- ✅ Video files can be streamed
- ✅ RTMP streams work (OBS integration)
- ✅ Frame rate consistent across devices
- ✅ Quality acceptable for installation viewing

### Phase 3 Complete When:
- ✅ Adaptive compression selects optimal method
- ✅ Graceful degradation under load
- ✅ Production-ready monitoring/logging
- ✅ Documentation complete

---

## Timeline Summary

| Phase | Duration | Dependencies | Deliverables |
|-------|----------|--------------|--------------|
| Phase 1: Core JPEG | 5 days | None | Working JPEG compression |
| Phase 2: Video Integration | 3 days | Phase 1 | Video streaming support |
| Phase 3: Advanced Features | 5 days | Phase 2 | Adaptive compression, monitoring |
| Testing & Deployment | 3 days | Phase 3 | Production deployment |
| **Total** | **16 days** | | **100×100 @ 8-10 fps** |

---

## Future Enhancements

### Post-Launch Improvements

**Near-term (1-3 months):**
- Web UI for quality/compression settings
- Real-time performance monitoring dashboard
- Automatic quality adjustment based on network conditions
- Frame caching for repeated content

**Medium-term (3-6 months):**
- Delta frame encoding for animations
- Hybrid compression (JPEG + RLE)
- GPU acceleration on server (NVIDIA NVENC)
- ESP32 hardware migration kit

**Long-term (6-12 months):**
- Custom optimized codec for LED displays
- WebRTC streaming support
- Cloud-based frame rendering service
- Mobile app control interface

### Research Items

**Explore:**
- AV1 codec viability (better compression than JPEG)
- ESP32-S3 with hardware H.264 support
- WebGPU for server-side frame processing
- Machine learning for compression optimization

---

## Resources

### Required Tools
- **FFmpeg** - Video decoding and processing
- **Sharp** - Image manipulation (already in project)
- **TJpgDec** - ESP8266 JPEG decoder library
- **PlatformIO** - Firmware build system

### Documentation Links
- TJpgDec Library: https://github.com/Bodmer/TJpg_Decoder
- JPEG Standard: https://en.wikipedia.org/wiki/JPEG
- ESP8266 Arduino Core: https://github.com/esp8266/Arduino
- Sharp Image Library: https://sharp.pixelplumbing.com/

### Learning Resources
- JPEG compression basics
- ESP8266 memory management
- UDP multicast optimization
- Video streaming protocols

---

## Appendix A: Code Structure

```
FataMorgana/
├── lib/
│   └── FataMorgana/
│       └── src/
│           ├── FataMorganaProtocol.h      (MODIFY: Add JPEG frame type)
│           ├── FataMorganaClient.h        (MODIFY: Add JPEG decode)
│           ├── FataMorganaClient.cpp      (MODIFY: Implement decode)
│           └── FataMorganaRenderer.h      (MODIFY: Add RGB888 support)
├── server/
│   ├── jpegFrameSender.js                 (NEW: JPEG compression)
│   ├── videoStreamer.js                   (MODIFY: Add JPEG support)
│   ├── gradientFrameServer.js             (MODIFY: Add JPEG endpoints)
│   └── adaptiveCompression.js             (NEW: Smart compression)
├── platformio.ini                          (MODIFY: Add TJpgDec dependency)
└── COMPRESSION_ROADMAP.md                  (THIS FILE)
```

---

## Appendix B: Memory Budget

**ESP8266 Memory Analysis:**

```
Total SRAM:           80 KB
System Reserved:      ~30 KB
──────────────────────────────
Available:            ~50 KB

Current Usage:
  Frame Buffer:       4,800 bytes
  NeoPixel Library:   300 bytes/100 LEDs
  WiFi Stack:         ~15 KB
  Application:        ~5 KB
──────────────────────────────
Current Free:         ~25 KB

With JPEG (100×100):
  Frame Buffer:       2,000 bytes (JPEG)
  Decode Buffer:      30,000 bytes (RGB888)
  NeoPixel Library:   300 bytes
  WiFi Stack:         ~15 KB
  Application:        ~5 KB
──────────────────────────────
Required Free:        ~47.3 KB (tight!)
```

**Mitigation Strategies:**
1. Free decode buffer immediately after rendering
2. Don't keep previous frame in memory
3. Disable verbose logging in production
4. Use heap fragmentation monitoring
5. Add graceful fallback to lower resolution

---

## Appendix C: Bandwidth Calculations

**100×100 @ 10 fps with JPEG:**

```
Frame size (JPEG):     2,000 bytes
Frames per second:     10 fps
Data per second:       20,000 bytes = 160 Kbps
Overhead (headers):    ~80 bytes/frame = 800 bytes/sec = 6.4 Kbps
──────────────────────────────────────────────
Total bandwidth:       ~166 Kbps per device

30 devices multicast:  166 Kbps (shared)
100 devices multicast: 166 Kbps (shared!)

WiFi capacity:         11 Mbps (802.11b)
                      54 Mbps (802.11g)
                      150+ Mbps (802.11n)
──────────────────────────────────────────────
Headroom:             66× on 802.11g ✅
```

**Multicast efficiency:** One stream serves unlimited devices!

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-06-04 | System | Initial roadmap created |

---

## Approval & Sign-off

**Technical Lead:** _________________  Date: __________

**Project Manager:** _________________  Date: __________

**Ready to Proceed:** ☐ Yes  ☐ No  ☐ Needs Review

---

**Next Steps:**
1. Review and approve roadmap
2. Set up development environment
3. Create feature branch: `feature/jpeg-compression`
4. Begin Phase 1 implementation
5. Schedule weekly progress reviews
