# TODO: Decoder Refactoring for Compositing Support

**Goal:** Enable users to composite FataMorgana frames with other content (artwork, effects) before rendering.

**Key Feature:** ARGB1555 format with transparency support

**Use Case:** Display base artwork, overlay FataMorgana network frames on top with transparent regions.

---

## Phase 1: Design Decisions

### Decision 1: Architecture Approach
- [ ] **Decide:** Keep both classes or breaking change?
  - **Option A:** Keep `FataMorganaClient` (integrated) + Add `FataMorganaDecoder` (flexible)
  - **Option B:** Replace Client with Decoder (breaking change)
  - **Option C:** Add `getPixels()` to existing Client
  - **Recommendation:** Option A (backward compatible)

### Decision 2: Transparency Default
- [ ] **Decide:** When using RGB332/RGB565 (no alpha channel), what should transparency be?
  - **Option A:** All opaque (assume no transparency)
  - **Option B:** All transparent (user must explicitly set opaque)
  - **Option C:** Magic color (e.g., black = transparent)
  - **Recommendation:** Option A (opaque by default for backward compat)

### Decision 3: Library Organization
- [ ] **Decide:** Where does Decoder class live?
  - **Option A:** In core FataMorgana library (alongside Client)
  - **Option B:** Separate `FataMorgana-Advanced` library
  - **Option C:** Replace Client entirely (v2.0.0 breaking change)
  - **Recommendation:** Option A (same library, more choice)

### Decision 4: BasicClient Example
- [ ] **Decide:** What should BasicClient demonstrate?
  - **Option A:** Keep using FataMorganaClient (simple, integrated)
  - **Option B:** Switch to FataMorganaDecoder (show best practices)
  - **Recommendation:** Option A (keep it simple for learning)

### Decision 5: Brightness Application
- [ ] **Decide:** Where should brightness be applied?
  - **Option A:** Decoder applies to decoded colors (current behavior)
  - **Option B:** User applies after compositing
  - **Option C:** Both (decoder has option, user can override)
  - **Recommendation:** Option A (decoder applies it)

---

## Phase 2: Protocol Changes

### Task 2.1: Add ARGB1555 Format
- [ ] Add constant `FATAMORGANA_ARGB1555 = 2` to `FataMorganaProtocol.h`
- [ ] Document bit layout in protocol header:
  ```
  Bit 15:    Alpha (0=transparent, 1=opaque)
  Bits 10-14: Red (5 bits, 0-31)
  Bits 5-9:   Green (5 bits, 0-31)
  Bits 0-4:   Blue (5 bits, 0-31)
  ```
- [ ] Update `fatamorgana_bytesPerPixel()` helper:
  ```cpp
  case FATAMORGANA_ARGB1555:
      return 2;
  ```
- [ ] Update `fatamorgana_rgbTypeName()` helper:
  ```cpp
  case FATAMORGANA_ARGB1555:
      return "ARGB1555";
  ```
- [ ] Update protocol documentation (FATAMORGANA_PROTOCOL.md)

### Task 2.2: Update FataMorganaColor Structure
- [ ] Add `transparent` field to `FataMorganaConfig.h`:
  ```cpp
  struct FataMorganaColor {
      uint8_t r;          // 0-255
      uint8_t g;          // 0-255
      uint8_t b;          // 0-255
      bool transparent;   // true if alpha=0 (for ARGB formats)
  };
  ```
- [ ] Add constructor for convenience:
  ```cpp
  FataMorganaColor(uint8_t r, uint8_t g, uint8_t b, bool transparent = false)
      : r(r), g(g), b(b), transparent(transparent) {}
  ```

---

## Phase 3: Renderer Changes

### Task 3.1: Extract Renderer Logic
- [ ] Review `FataMorganaRenderer` class
- [ ] Ensure it can decode to color array without NeoPixel
- [ ] Add ARGB1555 decoding in `decodePixel()`:
  ```cpp
  case FATAMORGANA_ARGB1555: {
      uint16_t pixel = frameBuffer[offset] | (frameBuffer[offset+1] << 8);
      bool alpha = (pixel & 0x8000) != 0;
      uint8_t r = ((pixel >> 10) & 0x1F) * 255 / 31;
      uint8_t g = ((pixel >> 5) & 0x1F) * 255 / 31;
      uint8_t b = (pixel & 0x1F) * 255 / 31;
      return FataMorganaColor(r, g, b, !alpha);  // Note: invert alpha for transparent flag
  }
  ```
- [ ] Verify transparency flag logic (alpha=0 → transparent=true)

---

## Phase 4: Create FataMorganaDecoder Class

### Task 4.1: Create Header File
- [ ] Create `lib/FataMorgana/src/FataMorganaDecoder.h`
- [ ] Define class with API:
  ```cpp
  class FataMorganaDecoder {
  public:
      FataMorganaDecoder(uint16_t pixelCount);

      // Lifecycle
      bool begin();
      void loop();

      // Access decoded pixels
      bool hasNewFrame();
      FataMorganaColor* getPixels();
      uint16_t getPixelCount();

      // Configuration (same as Client)
      void setRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
      void setRowMapping(uint16_t row, uint16_t pixels);
      void setColumnMapping(uint16_t col, uint16_t pixels);
      void setSerpentine(uint8_t mode);
      void setRotation(uint8_t rotation);
      void setFlip(bool x, bool y, bool z);
      void setSampleMode(uint8_t mode);
      void setBrightness(uint8_t brightness);

      // Stats
      uint32_t getDecodedFrames() const;
      uint32_t getAcceptedPackets() const;
      uint32_t getRejectedPackets() const;

      const FataMorganaMapping& getMapping() const;
  };
  ```

### Task 4.2: Implement Decoder
- [ ] Create `lib/FataMorgana/src/FataMorganaDecoder.cpp`
- [ ] Reuse UDP/frame logic from FataMorganaClient
- [ ] Store decoded pixels in internal `FataMorganaColor*` array
- [ ] Set `hasNewFrame()` flag when frame complete
- [ ] Apply brightness to decoded colors
- [ ] Don't depend on Adafruit_NeoPixel

### Task 4.3: Add to Main Header
- [ ] Add to `lib/FataMorgana/src/FataMorgana.h`:
  ```cpp
  #include "FataMorganaDecoder.h"
  ```

---

## Phase 5: Update FataMorganaClient (Backward Compatibility)

### Task 5.1: Keep Client Working
- [ ] **Do NOT break existing FataMorganaClient**
- [ ] Verify all existing examples still compile
- [ ] Client continues to own NeoPixel and auto-render
- [ ] Client ignores transparency (treats all as opaque)

### Task 5.2: Optional Enhancement
- [ ] **Optional:** Add `getPixels()` to Client for advanced users:
  ```cpp
  // In FataMorganaClient.h
  FataMorganaColor* getPixels();  // Access decoded pixels before render
  ```
- [ ] **Decision needed:** Is this useful or just confusing?

---

## Phase 6: Server Updates

### Task 6.1: Add ARGB1555 Encoding to Node.js Server
- [ ] Update `server/gradientFrameServer.js`
- [ ] Add ARGB1555 encoding function:
  ```javascript
  function encodeARGB1555(r, g, b, alpha) {
      const r5 = Math.round(r * 31 / 255);
      const g5 = Math.round(g * 31 / 255);
      const b5 = Math.round(b * 31 / 255);
      const a1 = alpha ? 1 : 0;
      return (a1 << 15) | (r5 << 10) | (g5 << 5) | b5;
  }
  ```
- [ ] Add pattern generator with transparency
- [ ] Add API parameter `rgbType` with values: `rgb332`, `rgb565`, `argb1555`
- [ ] Update server documentation

### Task 6.2: Test Patterns
- [ ] Create test pattern with transparent regions
- [ ] Create checkerboard (alternating opaque/transparent)
- [ ] Create alpha gradient test

---

## Phase 7: Examples

### Task 7.1: Keep Existing Examples Working
- [ ] **BasicClient** - No changes (uses FataMorganaClient)
- [ ] **CustomMapping** - No changes (uses FataMorganaClient)
- [ ] **RectangleWithTransforms** - No changes (uses FataMorganaClient)
- [ ] **WithWebInterface** - No changes (uses FataMorganaClient)
- [ ] **Verify all compile after refactoring**

### Task 7.2: Create New Decoder Example
- [ ] Create `examples/CompositeWithArtwork/CompositeWithArtwork.ino`
- [ ] Demonstrates FataMorganaDecoder usage
- [ ] Shows compositing with transparency
- [ ] Example base artwork (animated pattern)
- [ ] Handles ARGB1555 frames with transparency
- [ ] Clear comments explaining compositing logic

### Task 7.3: Create FastLED Example
- [ ] Create `examples/FastLEDIntegration/FastLEDIntegration.ino`
- [ ] Shows using Decoder with FastLED instead of NeoPixel
- [ ] Demonstrates library flexibility

---

## Phase 8: Documentation

### Task 8.1: Update API.md
- [ ] Add FataMorganaDecoder API documentation
- [ ] Document FataMorganaColor structure with transparency
- [ ] Add ARGB1555 format documentation
- [ ] Add compositing examples
- [ ] When to use Client vs Decoder

### Task 8.2: Update GETTING_STARTED.md
- [ ] Add section on Client vs Decoder choice
- [ ] Add compositing use case explanation

### Task 8.3: Create COMPOSITING_GUIDE.md
- [ ] Explain transparency concept
- [ ] ARGB1555 format details
- [ ] Example compositing techniques:
  - Basic transparency overlay
  - Alpha blending
  - Masking regions
  - Effects processing
- [ ] Performance considerations

### Task 8.4: Update CHANGELOG.md
- [ ] Add v1.1.0 section
- [ ] Document new FataMorganaDecoder class
- [ ] Document ARGB1555 support
- [ ] Document compositing capability
- [ ] Note backward compatibility maintained

### Task 8.5: Update Protocol Documentation
- [ ] Update FATAMORGANA_PROTOCOL.md with ARGB1555 spec
- [ ] Add bit layout diagram
- [ ] Add encoding/decoding examples
- [ ] Update frame size calculations

---

## Phase 9: Testing

### Task 9.1: Unit Tests (if applicable)
- [ ] Test ARGB1555 encoding/decoding
- [ ] Test transparency flag extraction
- [ ] Test color scaling (5-bit to 8-bit)
- [ ] Test all three RGB formats decode correctly

### Task 9.2: Integration Tests
- [ ] Test Decoder receives frames
- [ ] Test hasNewFrame() flag behavior
- [ ] Test getPixels() returns correct data
- [ ] Test transparency in decoded pixels
- [ ] Test brightness application to decoded colors

### Task 9.3: Hardware Tests
- [ ] Test compositing on actual hardware
- [ ] Verify transparency works as expected
- [ ] Test performance (frame rate with compositing)
- [ ] Test with ESP8266 and ESP32
- [ ] Test memory usage (decoder array)

### Task 9.4: Backward Compatibility Tests
- [ ] Verify existing Client examples compile
- [ ] Verify existing Client examples run unchanged
- [ ] Test RGB332 and RGB565 still work
- [ ] No breaking changes to existing API

---

## Phase 10: Library Metadata

### Task 10.1: Update library.json
- [ ] Bump version to 1.1.0
- [ ] Update description to mention compositing
- [ ] Add keywords: compositing, transparency, ARGB

### Task 10.2: Update library.properties
- [ ] Bump version to 1.1.0
- [ ] Update sentence to mention decoder

### Task 10.3: Update keywords.txt
- [ ] Add FataMorganaDecoder
- [ ] Add ARGB1555
- [ ] Add transparency-related keywords

---

## Phase 11: RectangleWithTransforms WiFiManager Update

### Task 11.1: Update RectangleWithTransforms.ino
- [ ] Replace hardcoded WiFi with WiFiManager
- [ ] Change AP name to "FataMorgana-Transform"
- [ ] Add `wifiManager.process()` in loop
- [ ] Update documentation header
- [ ] Test compilation

---

## Phase 12: Publish

### Task 12.1: Final Review
- [ ] All examples compile
- [ ] All documentation updated
- [ ] CHANGELOG complete
- [ ] No breaking changes verified
- [ ] Memory usage acceptable

### Task 12.2: Git Commit
- [ ] Commit with clear message about v1.1.0 features
- [ ] Tag as v1.1.0

### Task 12.3: Release
- [ ] Create GitHub release
- [ ] Update PlatformIO registry
- [ ] Update Arduino Library Manager
- [ ] Announce new compositing feature

---

## Open Questions

### Question 1: Performance
- **Q:** What's the performance impact of compositing 256+ pixels per frame?
- **A:** Need to test on ESP8266 (slower) vs ESP32

### Question 2: Memory
- **Q:** Does storing FataMorganaColor array (4 bytes × pixel count) fit in ESP8266 RAM?
- **A:** 256 pixels × 4 bytes = 1KB (should be fine, but test)

### Question 3: Advanced Compositing
- **Q:** Should we provide helper functions for common compositing operations?
- **Ideas:**
  - `blend(color1, color2, alpha)`
  - `applyMask(pixels, mask)`
  - `colorKey(pixels, keyColor)` (chroma key)
- **Decision:** Start simple, add if requested

### Question 4: Multiple Decoders
- **Q:** Can user run multiple FataMorganaDecoder instances?
- **Use case:** Receive from multiple multicast groups?
- **Decision:** Document limitations if any

### Question 5: WebUI for Decoder
- **Q:** Should FataMorganaWebUI work with Decoder?
- **A:** WebUI expects Client with NeoPixel. Either:
  - Keep WebUI for Client only
  - Create separate DecoderWebUI
  - Make WebUI work with both
- **Decision needed**

---

## Priority Order

1. **High Priority** (Core functionality)
   - Phase 2: Protocol changes (ARGB1555)
   - Phase 3: Renderer updates
   - Phase 4: Create Decoder class
   - Phase 8: Documentation

2. **Medium Priority** (Examples & Testing)
   - Phase 7: Examples
   - Phase 9: Testing
   - Phase 6: Server updates

3. **Low Priority** (Polish)
   - Phase 5: Optional Client enhancements
   - Phase 10: Metadata updates
   - Phase 11: WiFiManager update
   - Phase 12: Publish

---

## Notes

- Keep backward compatibility as top priority
- Document design decisions as we go
- Test on real hardware early
- Get user feedback on API before finalizing

---

**Created:** 2026-04-22
**Status:** Planning Phase
**Target Version:** 1.1.0
