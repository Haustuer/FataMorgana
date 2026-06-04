# Architecture: Decoder with External Renderer

**Date:** 2026-04-22
**Status:** Design Discussion
**Goal:** Enable compositing FataMorgana network frames with local artwork

---

## Problem Statement

### Use Case
User has:
- **100x100 pixel artwork** (10,000 physical LEDs)
- **10x10 FataMorgana region** from network (100 pixels)
- Wants to composite FataMorgana into a specific region of their artwork
- FataMorgana frames have transparency (ARGB1555 format)

### Why Integrated Renderer Doesn't Work

```cpp
// Current architecture - DOESN'T WORK for compositing
FataMorganaClient client(100, 12);  // Only knows about 100 LEDs!

// But user has 10,000 LEDs and needs to render all of them!
Adafruit_NeoPixel strip(10000, 12);  // Where does this go?
```

**Problem:**
- Client owns NeoPixel strip sized for FataMorgana region (100 LEDs)
- User needs to render full artwork (10,000 LEDs)
- No way to composite because renderer is wrong size

---

## Solution: External Renderer Architecture

### Core Principle
**Separation of Concerns:**
- **Decoder** - Receives network frames, decodes to XY buffer
- **User Code** - Owns LED array, handles compositing and rendering

### Component Responsibilities

```
┌─────────────────────────────────────────────────────────┐
│ FataMorganaDecoder                                      │
├─────────────────────────────────────────────────────────┤
│ • Receives UDP frames                                   │
│ • Decodes RGB data (RGB332/RGB565/ARGB1555)            │
│ • Applies mapping (rectangle/row/column)                │
│ • Stores in XY coordinate buffer (10x10 in example)    │
│ • Exposes decoded pixels via API                        │
│ • NO rendering - just provides data                     │
└─────────────────────────────────────────────────────────┘
                         ↓ getPixels()
┌─────────────────────────────────────────────────────────┐
│ User Code                                               │
├─────────────────────────────────────────────────────────┤
│ • Owns Adafruit_NeoPixel (full size: 10,000 LEDs)      │
│ • Owns artwork system (100x100 grid)                    │
│ • Compositing logic:                                    │
│   - Iterate all 10,000 LEDs                             │
│   - For region (45,45) to (54,54): check FataMorgana    │
│   - If FM pixel opaque: use FM color                    │
│   - If FM pixel transparent: use artwork color          │
│   - Rest: use artwork color                             │
│ • Calls strip.show()                                    │
└─────────────────────────────────────────────────────────┘
```

---

## API Design

### FataMorganaDecoder Class

```cpp
class FataMorganaDecoder {
public:
  // Constructor - pixel count is for the DECODED region
  FataMorganaDecoder(uint16_t pixelCount);

  // Lifecycle
  bool begin();
  void loop();  // Receives and decodes, does NOT render

  // Frame status
  bool hasNewFrame();
  uint32_t getFrameNumber();

  // Access decoded pixels in XY coordinate space
  FataMorganaColor* getPixels();           // Get full array
  FataMorganaColor getPixel(uint16_t x, uint16_t y);  // Get single pixel
  uint16_t getWidth();                     // Decoded region width
  uint16_t getHeight();                    // Decoded region height
  uint16_t getPixelCount();                // Total pixels in region

  // Configuration (same as current Client)
  void setRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void setRowMapping(uint16_t row, uint16_t pixels);
  void setColumnMapping(uint16_t col, uint16_t pixels);
  void setSerpentine(uint8_t mode);
  void setRotation(uint8_t rotation);
  void setFlip(bool x, bool y, bool z);
  void setSampleMode(uint8_t mode);
  void setBrightness(uint8_t brightness);  // Applied to decoded colors

  // Statistics
  uint32_t getDecodedFrames() const;
  uint32_t getAcceptedPackets() const;
  uint32_t getRejectedPackets() const;
  const FataMorganaMapping& getMapping() const;

private:
  // Internal XY buffer (width × height)
  FataMorganaColor* _xyBuffer;
  uint16_t _width, _height;
  uint16_t _pixelCount;

  // No Adafruit_NeoPixel member!
  // No rendering methods!
};
```

### FataMorganaColor Structure (Enhanced)

```cpp
struct FataMorganaColor {
  uint8_t r;          // 0-255
  uint8_t g;          // 0-255
  uint8_t b;          // 0-255
  bool transparent;   // true if alpha bit = 0 (ARGB1555 only)

  FataMorganaColor() : r(0), g(0), b(0), transparent(false) {}
  FataMorganaColor(uint8_t r, uint8_t g, uint8_t b, bool transparent = false)
    : r(r), g(g), b(b), transparent(transparent) {}
};
```

---

## Usage Examples

### Example 1: Simple Case (No Compositing)

Even for simple cases, user handles rendering explicitly.

```cpp
#include <Adafruit_NeoPixel.h>
#include <FataMorgana.h>

FataMorganaDecoder decoder(256);        // 16x16 region
Adafruit_NeoPixel strip(256, 12, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show();

  decoder.begin();
  decoder.setRectangle(0, 0, 16, 16);
}

void loop() {
  decoder.loop();

  if (decoder.hasNewFrame()) {
    FataMorganaColor* pixels = decoder.getPixels();

    // Simple rendering - direct mapping
    for (uint16_t i = 0; i < 256; i++) {
      strip.setPixelColor(i, pixels[i].r, pixels[i].g, pixels[i].b);
    }

    strip.show();
  }
}
```

**Lines of code:** ~25 (vs ~10 with integrated client)
**Trade-off:** More verbose, but gives full control

---

### Example 2: Compositing Case (The Goal!)

```cpp
#include <Adafruit_NeoPixel.h>
#include <FataMorgana.h>

// User's artwork system
class MyArtwork {
public:
  MyArtwork(uint16_t width, uint16_t height) : _w(width), _h(height) {}

  void update() {
    // Animate artwork
    _time++;
  }

  struct RGB { uint8_t r, g, b; };
  RGB getPixel(uint16_t x, uint16_t y) {
    // Return artwork pixel at (x, y)
    // This is your local content - animations, effects, etc.
    return {/* ... */};
  }

  uint16_t getWidth() { return _w; }
  uint16_t getHeight() { return _h; }

private:
  uint16_t _w, _h;
  uint32_t _time;
};

// Full LED array (100x100 = 10,000 LEDs)
Adafruit_NeoPixel strip(10000, 12, NEO_GRB + NEO_KHZ800);

// Artwork covers full display
MyArtwork artwork(100, 100);

// FataMorgana provides 10x10 region from network
FataMorganaDecoder decoder(100);  // 10x10 = 100 pixels

// Position where FataMorgana region appears in artwork
const uint16_t FM_OFFSET_X = 45;
const uint16_t FM_OFFSET_Y = 45;

void setup() {
  strip.begin();
  strip.setBrightness(80);
  strip.show();

  decoder.begin();
  decoder.setRectangle(0, 0, 10, 10);  // Subscribe to 10x10 from network
}

void loop() {
  // Update local artwork animation
  artwork.update();

  // Receive network frames
  decoder.loop();

  // Render full 100x100 grid to all 10,000 LEDs
  uint16_t artWidth = artwork.getWidth();
  uint16_t artHeight = artwork.getHeight();
  uint16_t fmWidth = decoder.getWidth();
  uint16_t fmHeight = decoder.getHeight();

  for (uint16_t artY = 0; artY < artHeight; artY++) {
    for (uint16_t artX = 0; artX < artWidth; artX++) {
      uint16_t ledIdx = artY * artWidth + artX;

      // Check if this artwork position overlaps FataMorgana region
      bool inFMRegion = (artX >= FM_OFFSET_X && artX < FM_OFFSET_X + fmWidth &&
                         artY >= FM_OFFSET_Y && artY < FM_OFFSET_Y + fmHeight);

      if (inFMRegion && decoder.hasNewFrame()) {
        // Get pixel from FataMorgana (in FM's local coordinates)
        uint16_t fmX = artX - FM_OFFSET_X;
        uint16_t fmY = artY - FM_OFFSET_Y;
        FataMorganaColor fmPixel = decoder.getPixel(fmX, fmY);

        if (!fmPixel.transparent) {
          // FataMorgana pixel is opaque - use it
          strip.setPixelColor(ledIdx, fmPixel.r, fmPixel.g, fmPixel.b);
        } else {
          // FataMorgana pixel is transparent - show artwork underneath
          MyArtwork::RGB artPixel = artwork.getPixel(artX, artY);
          strip.setPixelColor(ledIdx, artPixel.r, artPixel.g, artPixel.b);
        }
      } else {
        // Outside FataMorgana region - just show artwork
        MyArtwork::RGB artPixel = artwork.getPixel(artX, artY);
        strip.setPixelColor(ledIdx, artPixel.r, artPixel.g, artPixel.b);
      }
    }
  }

  strip.show();
}
```

**This is the key use case that requires external rendering!**

---

### Example 3: Multiple FataMorgana Regions

You could have multiple decoders for different regions:

```cpp
Adafruit_NeoPixel strip(10000, 12);
MyArtwork artwork(100, 100);

FataMorganaDecoder region1(100);  // 10x10 top-left
FataMorganaDecoder region2(100);  // 10x10 bottom-right

void setup() {
  strip.begin();

  region1.begin();
  region1.setRectangle(0, 0, 10, 10);   // Network position

  region2.begin();
  region2.setRectangle(10, 10, 10, 10); // Different network position
}

void loop() {
  artwork.update();
  region1.loop();
  region2.loop();

  // Render with both regions
  for (uint16_t y = 0; y < 100; y++) {
    for (uint16_t x = 0; x < 100; x++) {
      uint16_t ledIdx = y * 100 + x;

      // Check region 1 (at artwork position 10,10)
      if (x >= 10 && x < 20 && y >= 10 && y < 20 && region1.hasNewFrame()) {
        FataMorganaColor px = region1.getPixel(x - 10, y - 10);
        if (!px.transparent) {
          strip.setPixelColor(ledIdx, px.r, px.g, px.b);
          continue;
        }
      }

      // Check region 2 (at artwork position 80,80)
      if (x >= 80 && x < 90 && y >= 80 && y < 90 && region2.hasNewFrame()) {
        FataMorganaColor px = region2.getPixel(x - 80, y - 80);
        if (!px.transparent) {
          strip.setPixelColor(ledIdx, px.r, px.g, px.b);
          continue;
        }
      }

      // Default: artwork
      MyArtwork::RGB art = artwork.getPixel(x, y);
      strip.setPixelColor(ledIdx, art.r, art.g, art.b);
    }
  }

  strip.show();
}
```

---

## Helper Functions (Optional)

To reduce boilerplate, we could provide optional helper functions:

### Simple Render Helper

```cpp
// For simple cases: direct 1:1 mapping
void FataMorgana_renderDirect(FataMorganaDecoder& decoder,
                               Adafruit_NeoPixel& strip) {
  if (!decoder.hasNewFrame()) return;

  FataMorganaColor* pixels = decoder.getPixels();
  uint16_t count = decoder.getPixelCount();

  for (uint16_t i = 0; i < count; i++) {
    strip.setPixelColor(i, pixels[i].r, pixels[i].g, pixels[i].b);
  }

  strip.show();
}

// Usage
void loop() {
  decoder.loop();
  FataMorgana_renderDirect(decoder, strip);  // One line!
}
```

### Composite Render Helper

```cpp
// For compositing: overlay FM region onto artwork
template<typename ArtworkType>
void FataMorgana_renderComposite(
  FataMorganaDecoder& decoder,
  ArtworkType& artwork,
  Adafruit_NeoPixel& strip,
  uint16_t fmOffsetX,
  uint16_t fmOffsetY
) {
  uint16_t artW = artwork.getWidth();
  uint16_t artH = artwork.getHeight();
  uint16_t fmW = decoder.getWidth();
  uint16_t fmH = decoder.getHeight();

  for (uint16_t y = 0; y < artH; y++) {
    for (uint16_t x = 0; x < artW; x++) {
      uint16_t ledIdx = y * artW + x;

      // Check if in FM region
      if (x >= fmOffsetX && x < fmOffsetX + fmW &&
          y >= fmOffsetY && y < fmOffsetY + fmH &&
          decoder.hasNewFrame()) {

        FataMorganaColor fmPx = decoder.getPixel(x - fmOffsetX, y - fmOffsetY);

        if (!fmPx.transparent) {
          strip.setPixelColor(ledIdx, fmPx.r, fmPx.g, fmPx.b);
          continue;
        }
      }

      // Show artwork
      auto artPx = artwork.getPixel(x, y);
      strip.setPixelColor(ledIdx, artPx.r, artPx.g, artPx.b);
    }
  }

  strip.show();
}

// Usage
void loop() {
  artwork.update();
  decoder.loop();
  FataMorgana_renderComposite(decoder, artwork, strip, 45, 45);  // Simple!
}
```

**Note:** Helpers are optional - advanced users can write custom rendering.

---

## Coordinate Systems Explained

### Network Frame Space
```
Server sends 24x100 frame
┌────────────────────┐
│                    │
│  ┌──────┐          │
│  │10x10 │ ← Device subscribes via setRectangle(0,0,10,10)
│  └──────┘          │
│                    │
└────────────────────┘
```

### Decoder XY Space (What User Gets)
```
Decoder provides 10x10 buffer
Coordinates: (0,0) to (9,9)
┌──────────┐
│(0,0)     │
│          │
│      (9,9)│
└──────────┘
```

### Artwork Space (User's Local Content)
```
User's artwork is 100x100
┌────────────────────────────┐
│                            │
│      ┌──────────┐          │
│      │FM region │ ← At offset (45,45)
│      │10x10     │          │
│      └──────────┘          │
│                            │
└────────────────────────────┘
```

### LED Physical Space (Hardware)
```
10,000 physical LEDs
LED[0] ... LED[9999]

Mapping: ledIdx = artY * artWidth + artX
```

**User handles all coordinate mappings in their rendering loop!**

---

## Transforms (Serpentine, Rotation, Flip)

### Question: Where Do Transforms Apply?

**Current Thinking:** Transforms should apply to the **LED output**, not the decoder buffer.

**Why:**
- Decoder provides **logical XY coordinates** (always rectangular grid)
- User composites in **logical space** (easy to think about)
- Transforms are a **physical display concern**

### Problem with This Approach

If decoder handles transforms, user gets pre-transformed XY buffer:
```cpp
// Decoder applies serpentine internally
decoder.setSerpentine(SERPENTINE_HORIZONTAL);
FataMorganaColor* xy = decoder.getPixels();  // Already serpentine!
```

**Issue:** User's artwork is NOT serpentine. Can't composite easily.

### Better Approach: User Applies Transforms During Render

```cpp
// Decoder provides logical XY grid (no transforms)
FataMorganaColor* xy = decoder.getPixels();

// User applies transforms when mapping to LEDs
for (uint16_t ledIdx = 0; ledIdx < count; ledIdx++) {
  uint16_t x, y;
  ledIndexToXY(ledIdx, x, y, width, SERPENTINE_HORIZONTAL);
  strip.setPixelColor(ledIdx, xy[y * width + x].r, ...);
}
```

### Provide Transform Helper

```cpp
// Library provides this helper function
void ledIndexToXY(uint16_t ledIdx, uint16_t& x, uint16_t& y,
                  uint16_t width, uint16_t height,
                  uint8_t serpentine, uint8_t rotation,
                  bool flipX, bool flipY, bool flipZ) {
  // Apply serpentine
  if (serpentine == SERPENTINE_HORIZONTAL) {
    y = ledIdx / width;
    x = (y % 2 == 0) ? (ledIdx % width) : (width - 1 - (ledIdx % width));
  } else if (serpentine == SERPENTINE_VERTICAL) {
    x = ledIdx / height;
    y = (x % 2 == 0) ? (ledIdx % height) : (height - 1 - (ledIdx % height));
  } else {
    x = ledIdx % width;
    y = ledIdx / width;
  }

  // Apply rotation
  // ... rotation logic

  // Apply flips
  if (flipX) x = width - 1 - x;
  if (flipY) y = height - 1 - y;
  if (flipZ) std::swap(x, y);
}
```

**Decision Needed:** Should decoder store transform settings, or should user manage them?

---

## Memory Considerations

### Decoder Memory Usage

```cpp
FataMorganaDecoder decoder(256);  // 16x16 region
```

**Memory required:**
```
XY Buffer: 256 pixels × sizeof(FataMorganaColor)
         = 256 × 4 bytes = 1,024 bytes

Frame buffer: 4800 bytes max (protocol limit)

Total: ~6 KB
```

**ESP8266 RAM:** 80 KB available
**Verdict:** ✅ Should be fine for typical sizes

### Compositing Memory

For compositing case:
```cpp
Adafruit_NeoPixel strip(10000, 12);  // 10,000 LEDs
```

**Memory required:**
```
NeoPixel internal buffer: 10,000 × 3 bytes = 30 KB

Decoder XY buffer: 100 × 4 bytes = 400 bytes

Total: ~30.4 KB
```

**ESP8266 RAM:** 80 KB available
**Verdict:** ✅ Still reasonable, but getting tight

**Recommendation:** Test on actual hardware early!

---

## Performance Considerations

### Rendering Loop Performance

```cpp
// Worst case: 100x100 artwork, 10x10 FM region
for (uint16_t y = 0; y < 100; y++) {         // 100 iterations
  for (uint16_t x = 0; x < 100; x++) {       // 100 iterations
    // 10,000 total iterations per frame
    // Inside loop: coordinate checks, color assignment
  }
}
```

**Operations per frame:** ~10,000 iterations
**ESP8266 @ 80 MHz:** Should handle this easily
**Expected FPS:** 30-60 fps (depends on network frame rate)

**Optimization:** Only iterate FM region if that's all that changed:
```cpp
// Smart update: only redraw changed regions
if (decoder.hasNewFrame()) {
  // Redraw FM region only
  for (uint16_t fmY = 0; fmY < 10; fmY++) {
    for (uint16_t fmX = 0; fmX < 10; fmX++) {
      // 100 iterations instead of 10,000!
    }
  }
} else if (artwork.hasChanged()) {
  // Redraw artwork only
}
```

---

## Migration Path

### For Existing Users (Backward Compatibility)

**Option 1:** Keep `FataMorganaClient` unchanged
```cpp
// Still works for simple cases
FataMorganaClient client(256, 12);  // Integrated rendering
```

**Option 2:** Deprecate `FataMorganaClient`, provide migration guide
```cpp
// Old (v1.0)
FataMorganaClient client(256, 12);
client.loop();  // Auto-renders

// New (v1.1)
FataMorganaDecoder decoder(256);
Adafruit_NeoPixel strip(256, 12);
decoder.loop();
// ... user renders ...
strip.show();
```

**Recommendation:** Option 1 (keep both classes for now)

---

## Open Questions

### Q1: Should Decoder Store Transform Settings?

**Option A:** Yes, store but don't apply
```cpp
decoder.setSerpentine(SERPENTINE_HORIZONTAL);  // Stored
uint8_t serp = decoder.getSerpentine();        // User queries and applies
```

**Option B:** No, user manages entirely
```cpp
uint8_t mySerpentine = SERPENTINE_HORIZONTAL;
ledIndexToXY(ledIdx, x, y, w, h, mySerpentine);
```

**Recommendation:** Option A (decoder stores config, makes migration easier)

---

### Q2: Should We Provide `renderDirect()` Helper?

**Pros:**
- Reduces boilerplate for simple cases
- Easier migration from Client

**Cons:**
- Still requires NeoPixel dependency in core library
- Defeats the purpose of removing dependency

**Recommendation:** Provide as **example code**, not in core library

---

### Q3: Multiple Multicast Groups?

Can user run multiple decoders on different multicast groups?

```cpp
FataMorganaDecoder decoder1(100);  // Group 239.255.42.1
FataMorganaDecoder decoder2(100);  // Group 239.255.42.2 ???
```

**Answer:** Need to support custom multicast address:
```cpp
decoder1.begin();  // Default group
decoder2.begin(IPAddress(239, 255, 42, 2));  // Custom group
```

---

## Summary

### Architecture Decision: External Renderer

**Why:**
1. ✅ User's LED array can be larger than FataMorgana region
2. ✅ Enables compositing with local artwork
3. ✅ Gives user full control over rendering
4. ✅ Removes NeoPixel dependency from core
5. ✅ More flexible for non-NeoPixel use cases (FastLED, custom drivers)

**Trade-offs:**
1. ❌ More boilerplate for simple cases (~15 extra lines)
2. ❌ Steeper learning curve
3. ✅ But enables use cases that are impossible with integrated rendering

### Next Steps

1. Implement `FataMorganaDecoder` class
2. Add ARGB1555 protocol support
3. Create compositing example
4. Provide helper functions (as examples, not core library)
5. Test memory usage on ESP8266
6. Test performance with 100x100 compositing
7. Decide on backward compatibility (keep Client or deprecate)

---

**This architecture enables the compositing use case that motivated the refactoring!**
