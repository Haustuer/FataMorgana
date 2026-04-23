# FataMorgana Refactoring Plan
## Architectural Evolution for Distributed Pixel Distribution System

**Version:** 1.0
**Date:** 2026-04-23
**Status:** Planning Phase

---

## Executive Summary

FataMorgana is evolving from a simple LED library into a **distributed pixel distribution system** supporting heterogeneous consumers and large-scale deployments. This document outlines the architectural refactoring needed to support:

1. **Desert Installation**: Central control of hundreds of distributed nodes across wide areas
2. **Man Cave Ecosystem**: Multiple devices subscribing to the same stream for different purposes
3. **External Users**: Flexible architecture supporting various hardware and use cases

---

## Use Cases

### Use Case 1: Desert Light Installation

**Description:** Large-scale outdoor installation with centralized control

**Requirements:**
- Central control station managing 100-1000+ "dumb" client nodes
- Remote configuration and reconfiguration of all devices
- Wide area distribution (potentially km² coverage)
- One picture split across many physical nodes
- Bulk device management and monitoring
- Health monitoring and diagnostics
- OTA firmware updates

**Constraints:**
- ESP8266 hardware (memory-constrained)
- Potentially unreliable wireless connectivity
- Power management considerations
- Environmental durability

### Use Case 2: Man Cave / Ambilight Ecosystem

**Description:** Multiple heterogeneous devices consuming same pixel stream

**Subscribers:**
- Single-pixel lamps (sample one coordinate)
- Ambilight strips (edge pixels for TV backlighting)
- Matrix arrays (background layer with static foreground overlay)
- Non-LED devices (home automation based on average colors)
- Audio visualization (react to color data)
- Data logging / analysis

**Requirements:**
- Multiple devices doing different things with same data stream
- Efficient per-device region extraction
- Post-processing capabilities (blur, color analysis, etc.)
- Low latency for reactive applications
- Support for non-NeoPixel hardware (FastLED, APA102, custom)

### Use Case 3: External User Flexibility

**Description:** Library users with diverse hardware and requirements

**User Types:**
- Beginners: Simple setup with NeoPixels
- Advanced: Custom LED protocols, exotic hardware
- Integrators: Embedding in larger systems
- Artists: Creative effects and transformations

**Requirements:**
- Clear, documented extension points
- Multiple abstraction levels (simple → advanced)
- Examples for common scenarios
- Backwards compatibility where possible

---

## Current Architecture

### Strengths
- Efficient memory usage (decode-inline)
- Good performance on ESP8266
- Solid protocol implementation
- Working discovery and identify features
- Web UI for device configuration
- Out-of-bounds handling modes

### Limitations
- Tightly coupled to Adafruit_NeoPixel
- No abstraction between protocol and rendering
- Limited to LED hardware (can't do color analysis, etc.)
- No central management system
- No bulk configuration capabilities
- Difficult for external users to extend
- No config persistence (EEPROM)

### Current Flow
```
UDP Packets → FataMorganaClient → FataMorganaRenderer → Adafruit_NeoPixel
                                        ↓
                                  (decode inline)
```

---

## Target Architecture

### Layer 1: Protocol Core (Hardware-Independent)
**Purpose:** Pure UDP frame assembly, no hardware dependencies

```cpp
class FataMorganaProtocol {
  uint8_t* frameBuffer;
  uint16_t width, height;
  uint8_t rgbType;

  void handleUDPPacket(const uint8_t* packet, size_t length);
  bool isFrameComplete();
  const uint8_t* getFrameBuffer();
};
```

**Responsibilities:**
- UDP packet reception and assembly
- Frame counter tracking
- Chunk validation
- Discovery/Identify protocol handling
- Config protocol handling

### Layer 2: Decoder (Output-Agnostic)
**Purpose:** Pixel decoding without hardware assumptions

```cpp
class FataMorganaDecoder {
  RGB888 getPixel(uint16_t x, uint16_t y);
  RGB888 getPixelMapped(const Mapping& map, uint16_t ledIndex);
  void decodeRegion(RGB888* output, const Rect& region);
  RGB888 sampleInterpolated(float x, float y);
};
```

**Modes:**
- **Inline Mode** (ESP8266): Decode on-demand, no buffer
- **Buffered Mode** (ESP32): Full RGB888 decode for post-processing

**Responsibilities:**
- RGB332/RGB565 decoding
- Coordinate mapping
- Out-of-bounds handling
- Interpolation
- Transform application

### Layer 3: Renderer Abstraction
**Purpose:** Hardware-agnostic output interface

```cpp
class ILEDRenderer {
  virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;
  virtual void show() = 0;
  virtual uint16_t numPixels() = 0;
  virtual void setBrightness(uint8_t brightness) = 0;
};
```

**Built-in Implementations:**
- `NeoPixelRenderer` (Adafruit_NeoPixel wrapper)
- `FastLEDRenderer` (FastLED wrapper)
- `APA102Renderer` (SPI-based LEDs)
- `RawBufferRenderer` (RGB888 buffer output)

**User Extensions:**
- Custom LED protocols
- Virtual displays
- Data analysis
- Network forwarding

### Layer 4: High-Level Clients
**Purpose:** Convenience APIs for common use cases

```cpp
// Full-featured client (current use case)
class FataMorganaClient {
  FataMorganaProtocol protocol;
  FataMorganaDecoder decoder;
  ILEDRenderer* renderer;
};

// Specialized clients
class FataMorganaSinglePixel {
  RGB888 getPixelAt(uint16_t x, uint16_t y);
};

class FataMorganaAmbilight {
  void getEdges(RGB888* top, RGB888* bottom, RGB888* left, RGB888* right);
};

class FataMorganaAnalyzer {
  RGB888 getAverageColor(const Rect& region);
  ColorHistogram getHistogram();
};
```

### Layer 5: Management System
**Purpose:** Central control and monitoring (server-side)

**Components:**
- Device registry and discovery
- Bulk configuration deployment
- Visual layout designer
- Health monitoring dashboard
- Firmware OTA updates
- Test pattern generation

---

## Implementation Phases

### Phase 1: Core Refactoring (Weeks 1-2)
**Goal:** Separate decoder from renderer without breaking existing API

**Tasks:**
1. Extract decoder logic into separate class
   - Move `decodePixel()` to `FataMorganaDecoder`
   - Move `getPixelAt()` with OOB handling
   - Keep existing API working

2. Add renderer abstraction
   - Define `ILEDRenderer` interface
   - Implement `NeoPixelRenderer` wrapper
   - Make `FataMorganaRenderer` use interface

3. Maintain backwards compatibility
   - Existing code still works: `FataMorganaClient client(256, 12);`
   - Add new constructor: `FataMorganaClient client(256, 12, new FastLEDRenderer());`

**Deliverables:**
- Refactored core with separated concerns
- All existing examples still work
- New renderer examples (FastLED, raw buffer)
- Updated documentation

**Success Criteria:**
- All existing tests pass
- No memory overhead increase
- No performance regression
- Examples compile and run

### Phase 2: Protocol Extensions (Weeks 3-4)
**Goal:** Add remote configuration and persistence

**Tasks:**
1. Implement `CONFIG_SET_MAPPING` protocol
   ```cpp
   // Server sends mapping config
   void sendMappingConfig(IPAddress device, MappingConfig config);

   // Device receives and applies
   void handleSetMappingFrame(const uint8_t* payload);
   ```

2. Add EEPROM persistence
   ```cpp
   void saveConfigToEEPROM();
   void loadConfigFromEEPROM();
   bool hasStoredConfig();
   ```

3. Add config acknowledgment
   - Device confirms config applied
   - Server tracks which devices are configured

4. Extend discovery response
   - Include config version/timestamp
   - Include "needs configuration" flag

**Deliverables:**
- Remote config protocol implementation
- EEPROM storage for all config parameters
- Config versioning system
- Updated protocol documentation

**Success Criteria:**
- Devices persist config across reboots
- Remote config reliably updates devices
- Config conflicts detected and resolved

### Phase 3: Specialized Clients (Weeks 5-6)
**Goal:** Support heterogeneous consumers

**Tasks:**
1. Implement `FataMorganaSinglePixel`
   ```cpp
   class FataMorganaSinglePixel {
     void subscribe(uint16_t x, uint16_t y);
     RGB888 getColor();
     bool hasNewFrame();
   };
   ```

2. Implement `FataMorganaAmbilight`
   ```cpp
   class FataMorganaAmbilight {
     void setEdgeConfig(uint8_t top, uint8_t bottom, uint8_t left, uint8_t right);
     void getTopEdge(RGB888* buffer, uint16_t count);
     void getBottomEdge(RGB888* buffer, uint16_t count);
     // etc.
   };
   ```

3. Implement `FataMorganaAnalyzer`
   ```cpp
   class FataMorganaAnalyzer {
     RGB888 getAverageColor(const Rect& region);
     RGB888 getDominantColor(const Rect& region);
     float getBrightness(const Rect& region);
   };
   ```

4. Create examples for each client type
   - Single LED lamp example
   - Ambilight strip example
   - Home automation trigger example

**Deliverables:**
- Three specialized client classes
- Examples for each
- Documentation for extension
- Performance benchmarks

**Success Criteria:**
- Single pixel client uses <1KB RAM
- Ambilight client processes edges in <10ms
- Examples work on ESP8266

### Phase 4: Central Management System (Weeks 7-10)
**Goal:** Build control center for desert installation

**Tasks:**
1. Enhance server with device registry
   ```javascript
   class DeviceRegistry {
     devices: Map<IP, DeviceInfo>;

     addDevice(deviceInfo);
     updateDevice(ip, updates);
     getDevicesByStatus(status);
     getBulkStats();
   }
   ```

2. Build visual layout designer
   - Canvas-based device placement
   - Drag-and-drop positioning
   - Automatic mapping calculation
   - Export/import layouts

3. Implement bulk operations
   ```javascript
   // Select devices by criteria
   const outdoorDevices = registry.query({
     location: 'outdoor'
   });

   // Apply config to all
   bulkConfig(outdoorDevices, {
     brightness: 200,
     gamma: 2.8,
     oobMode: CLAMP
   });
   ```

4. Add health monitoring
   - Last-seen timestamps
   - Packet loss statistics
   - Frame rate tracking
   - Alert system for offline devices

5. Test pattern system
   - Checkerboard for alignment
   - Rainbow for color calibration
   - Sequential flash for identification
   - Custom test images

**Deliverables:**
- Enhanced web control panel
- Device registry system
- Visual layout designer
- Bulk configuration tools
- Health monitoring dashboard
- Test pattern generator

**Success Criteria:**
- Manage 100+ devices efficiently
- Bulk config updates complete in <10s
- Visual layout designer intuitive to use
- Health dashboard updates in real-time

### Phase 5: Advanced Features (Weeks 11-12)
**Goal:** Production-ready enhancements

**Tasks:**
1. OTA firmware updates
   - HTTP-based OTA
   - Bulk update scheduling
   - Rollback capability
   - Update progress monitoring

2. Mesh/relay support
   - ESP-NOW for local clusters
   - UDP relay for extended range
   - Multi-hop routing

3. Configuration templates
   - "Row mode, 100 devices, auto-assign"
   - "Matrix grid, 10x10"
   - "Circular arrangement"
   - Custom template editor

4. Performance optimizations
   - Frame delta compression
   - Smart multicast groups
   - Adaptive frame rate

5. Security enhancements
   - Config authentication
   - Encrypted control commands
   - Access control lists

**Deliverables:**
- OTA update system
- Mesh networking support
- Config template library
- Performance improvements
- Security framework

**Success Criteria:**
- OTA updates reliable across 100+ devices
- Mesh extends range by 3x
- Templates reduce setup time by 90%
- Frame compression saves 30% bandwidth

---

## Technical Decisions

### Memory Strategy

**Problem:** ESP8266 has ~40-50KB available RAM

**Solution:** Dual-mode architecture

```cpp
// Compile-time configuration
#ifdef FATAMORGANA_LIGHTWEIGHT_MODE
  // Inline decode, minimal features
  // Memory: frameBuffer only (~2-5KB)
  #define NO_RGB888_BUFFER
#else
  // Full decode buffer for post-processing
  // Memory: frameBuffer + RGB888 buffer (~10KB)
  #define ENABLE_RGB888_BUFFER
#endif
```

**Default Mode Selection:**
- ESP8266: LIGHTWEIGHT_MODE
- ESP32: Full mode
- User-configurable via platformio.ini

### Renderer Abstraction Method

**Decision:** Template-based with virtual fallback

**Rationale:**
- Templates: Zero overhead for performance-critical paths
- Virtual: Flexibility for runtime renderer switching
- Best of both worlds

```cpp
// Template version (zero overhead)
template<typename Renderer>
class FataMorganaClient { };

// Virtual version (flexibility)
class FataMorganaClient {
  ILEDRenderer* renderer;
};
```

**Recommendation:** Provide both, let user choose

### Configuration Persistence

**Decision:** EEPROM with wear-leveling

**Format:**
```cpp
struct StoredConfig {
  uint32_t magic;      // "FTMG"
  uint16_t version;    // Config version
  uint16_t checksum;   // CRC16
  FataMorganaMapping mapping;
  uint8_t reserved[32]; // Future expansion
};
```

**Wear-Leveling:** Rotate through 4 EEPROM blocks

### Protocol Extensions

**New Config Frame SubTypes:**
```cpp
CONFIG_SUBTYPE_DISCOVERY = 0      // Existing
CONFIG_SUBTYPE_SET_MAPPING = 1    // NEW
CONFIG_SUBTYPE_SET_BRIGHTNESS = 2 // NEW
CONFIG_SUBTYPE_IDENTIFY = 3       // Existing
CONFIG_SUBTYPE_RESET = 4          // NEW
CONFIG_SUBTYPE_OTA_START = 5      // NEW
```

**Backwards Compatibility:** Devices ignore unknown subtypes

---

## Library Structure

```
FataMorgana/
├── src/
│   ├── core/
│   │   ├── FataMorganaProtocol.h/cpp      # UDP frame assembly
│   │   ├── FataMorganaDecoder.h/cpp       # RGB decoding
│   │   ├── FataMorganaMapping.h/cpp       # Coordinate transforms
│   │   └── FataMorganaConfig.h            # Config structures
│   ├── renderers/
│   │   ├── ILEDRenderer.h                 # Interface
│   │   ├── NeoPixelRenderer.h/cpp         # Adafruit_NeoPixel
│   │   ├── FastLEDRenderer.h/cpp          # FastLED
│   │   ├── APA102Renderer.h/cpp           # SPI LEDs
│   │   └── RawBufferRenderer.h/cpp        # RGB888 output
│   ├── clients/
│   │   ├── FataMorganaClient.h/cpp        # Standard client
│   │   ├── FataMorganaSinglePixel.h/cpp   # Minimal client
│   │   ├── FataMorganaAmbilight.h/cpp     # Edge extraction
│   │   └── FataMorganaAnalyzer.h/cpp      # Color analysis
│   ├── management/
│   │   ├── FataMorganaConfigProtocol.h/cpp # Remote config
│   │   ├── FataMorganaEEPROM.h/cpp         # Persistence
│   │   └── FataMorganaOTA.h/cpp            # OTA updates
│   └── FataMorgana.h                      # Main include
├── examples/
│   ├── 01_BasicNeoPixel/                  # Simple example
│   ├── 02_CustomRenderer/                 # FastLED example
│   ├── 03_SinglePixelLamp/                # Minimal client
│   ├── 04_AmbilightStrip/                 # Edge extraction
│   ├── 05_ColorAnalyzer/                  # Automation
│   ├── 06_DesertNode/                     # Remote config
│   └── 07_ManCaveEcosystem/               # Multiple consumers
├── server/
│   ├── gradientFrameServer.js             # Enhanced server
│   ├── deviceRegistry.js                  # Device management
│   ├── bulkConfig.js                      # Bulk operations
│   ├── layoutDesigner.html                # Visual designer
│   └── healthMonitor.html                 # Dashboard
├── docs/
│   ├── ARCHITECTURE.md                    # Architecture overview
│   ├── PROTOCOL_EXTENSIONS.md             # New protocol features
│   ├── RENDERER_GUIDE.md                  # Custom renderer guide
│   ├── DESERT_DEPLOYMENT.md               # Large-scale guide
│   ├── AMBILIGHT_GUIDE.md                 # Multi-consumer guide
│   └── MIGRATION_GUIDE.md                 # v1 → v2 migration
└── tests/
    ├── unit/                              # Unit tests
    ├── integration/                       # Integration tests
    └── performance/                       # Benchmarks
```

---

## Migration Strategy

### Backwards Compatibility

**Existing Code:**
```cpp
// This MUST continue to work
FataMorganaClient client(256, 12);
client.begin();
client.setRectangle(0, 0, 16, 16);
```

**Strategy:**
1. Keep old API as default
2. New features opt-in
3. Deprecation warnings (not errors)
4. Migration guide with examples

### Migration Path

**Version 1.x → 2.0:**

```cpp
// Old (still works in 2.0)
#include <FataMorgana.h>
FataMorganaClient client(256, 12);

// New (recommended in 2.0)
#include <FataMorgana.h>
NeoPixelRenderer renderer(256, 12);
FataMorganaClient client(renderer);
```

**Breaking Changes:**
- None in core API
- Advanced features require new includes
- WebUI library remains separate

---

## Success Metrics

### Performance Benchmarks

**Target Performance (ESP8266 @ 80MHz):**
- Frame assembly: <5ms
- Inline decode + render: <10ms (RGB332, 256 pixels)
- Full decode: <20ms (RGB565, 256 pixels)
- Config update: <100ms
- Memory usage: <5KB overhead

### Scalability Targets

**Desert Installation:**
- Support 1000+ devices
- Bulk config update: <30s for 100 devices
- Health monitoring: <1s refresh rate
- Discovery: Find all devices in <5s

### User Experience Goals

**Ease of Use:**
- Beginner example: <10 lines of code
- Custom renderer: <50 lines of code
- Setup time: <5 minutes for first device
- Reconfiguration: <30s per device via web UI

---

## Risks and Mitigations

### Risk 1: Memory Overhead
**Impact:** High
**Probability:** Medium

**Mitigation:**
- Dual-mode architecture (lightweight/full)
- Extensive memory profiling
- Compile-time configuration options
- Clear documentation of requirements

### Risk 2: Backwards Compatibility
**Impact:** High
**Probability:** Low

**Mitigation:**
- Keep old API working
- Extensive regression testing
- Migration guide with examples
- Deprecation warnings (not removal)

### Risk 3: Performance Regression
**Impact:** Medium
**Probability:** Low

**Mitigation:**
- Benchmark every phase
- Template-based renderers (zero overhead)
- Optimize critical paths
- Performance regression tests

### Risk 4: Complexity for Users
**Impact:** Medium
**Probability:** Medium

**Mitigation:**
- Multiple abstraction levels
- Rich examples for common cases
- Clear documentation
- Video tutorials

### Risk 5: Protocol Version Conflicts
**Impact:** Medium
**Probability:** Medium

**Mitigation:**
- Protocol version negotiation
- Backwards compatibility in server
- Clear version reporting
- Migration tools

---

## Timeline

### Phase 1: Core Refactoring
**Duration:** 2 weeks
**Effort:** 60 hours
**Deliverable:** Refactored core with renderer abstraction

### Phase 2: Protocol Extensions
**Duration:** 2 weeks
**Effort:** 50 hours
**Deliverable:** Remote config and persistence

### Phase 3: Specialized Clients
**Duration:** 2 weeks
**Effort:** 40 hours
**Deliverable:** SinglePixel, Ambilight, Analyzer clients

### Phase 4: Central Management
**Duration:** 4 weeks
**Effort:** 100 hours
**Deliverable:** Control center for desert installation

### Phase 5: Advanced Features
**Duration:** 2 weeks
**Effort:** 60 hours
**Deliverable:** OTA, mesh, templates, security

**Total Duration:** 12 weeks
**Total Effort:** 310 hours

---

## Open Questions

1. **Memory vs Features:** Should we make lightweight mode the default, or offer both equally?

2. **Configuration Frequency:** In desert installation, how often will bulk reconfig happen? Affects optimization priorities.

3. **Man Cave Distribution:** Will multiple devices share same ESP, or are they separate? Affects local distribution strategy.

4. **External User Hardware:** What's the minimum target? ESP8266 only, or also ESP32, RP2040, etc.?

5. **Central Control Infrastructure:** Build from scratch or integrate with existing (Home Assistant, Node-RED, etc.)?

6. **Protocol Evolution:** How to handle future protocol changes while maintaining compatibility?

7. **Security Model:** Required for desert installation? What level (encryption, auth, ACLs)?

8. **Mesh Topology:** Star, mesh, hybrid? Affects relay implementation.

9. **OTA Strategy:** HTTP pull, or push-based? Affects server requirements.

10. **Testing Strategy:** Hardware-in-loop tests? Simulation? Both?

---

## Next Steps

### Immediate Actions

1. **Validate Architecture** (Week 0)
   - Review this plan with stakeholders
   - Resolve open questions
   - Prioritize phases based on urgency

2. **Prototype Phase 1** (Week 1)
   - Create renderer interface
   - Extract decoder logic
   - Build FastLED example
   - Measure memory/performance impact

3. **Community Feedback** (Week 1)
   - Share architecture proposal
   - Gather external user requirements
   - Identify edge cases

4. **Finalize Plan** (Week 2)
   - Incorporate feedback
   - Lock scope for v2.0
   - Create detailed task breakdown

### Long-term Roadmap

**v2.0 (12 weeks):** Refactored architecture, central management
**v2.1 (+4 weeks):** Mesh networking, OTA updates
**v2.2 (+4 weeks):** Advanced effects, compression
**v3.0 (+8 weeks):** ESP32-S3 support, PSRAM utilization, higher resolutions

---

## Conclusion

This refactoring transforms FataMorgana from an LED library into a comprehensive distributed pixel distribution platform. The phased approach ensures:

- **Backwards compatibility** for existing users
- **Flexibility** for diverse use cases (desert, man cave, custom)
- **Scalability** for large deployments
- **Extensibility** for external developers

The architecture supports both memory-constrained ESP8266 nodes and feature-rich ESP32 applications, while providing clear abstraction layers for users at all skill levels.

**Key Success Factors:**
- Maintain zero-overhead option for simple use cases
- Provide clear migration path
- Rich examples and documentation
- Central management tools for scale
- Active community engagement

---

## Appendices

### Appendix A: Example Use Cases

**Desert Installation Config:**
```cpp
// Node firmware
#define FATAMORGANA_LIGHTWEIGHT_MODE
#include <FataMorgana.h>

FataMorganaClient client(256, 12);
client.enableRemoteConfig(true);
client.enableEEPROM(true);

void setup() {
  WiFi.begin(SSID, PASSWORD);
  client.begin();
  // Config loaded from EEPROM or received remotely
}

void loop() {
  client.loop();
}
```

**Man Cave Single Pixel Lamp:**
```cpp
#include <FataMorgana.h>

FataMorganaSinglePixel lamp;

void setup() {
  WiFi.begin(SSID, PASSWORD);
  lamp.begin();
  lamp.subscribe(10, 50);  // x=10, y=50
}

void loop() {
  lamp.update();
  if (lamp.hasNewColor()) {
    RGB888 color = lamp.getColor();
    myLED.setRGB(color.r, color.g, color.b);
  }
}
```

**Man Cave Ambilight:**
```cpp
#include <FataMorgana.h>

FataMorganaAmbilight ambient;
FastLEDRenderer renderer(NUM_LEDS);

void setup() {
  WiFi.begin(SSID, PASSWORD);
  ambient.begin(renderer);
  ambient.setEdgePixels(10, 10, 5, 5); // top, bottom, left, right
}

void loop() {
  ambient.update();
  renderer.show();
}
```

### Appendix B: Protocol Extensions Detail

**CONFIG_SET_MAPPING Frame Format:**
```
Header (8 bytes):
  [0] FrameType = 0 (CONFIG)
  [1] FrameCounter
  [2] ChunkIndex = 0
  [3] SubType = 1 (SET_MAPPING)
  [4-5] Reserved
  [6-7] Reserved

Payload (28 bytes):
  [8] mode (0=row, 1=column, 2=rectangle)
  [9] sampleMode (0=pixel, 1=interpolated)
  [10-11] rowIndex / columnIndex (uint16_t LE)
  [12-13] linePixels (uint16_t LE)
  [14-15] rectX (uint16_t LE)
  [16-17] rectY (uint16_t LE)
  [18-19] rectWidth (uint16_t LE)
  [20-21] rectHeight (uint16_t LE)
  [22] serpentine (0=none, 1=horizontal, 2=vertical)
  [23] rotation (0-3)
  [24] transform byte (bits: flipX, flipY, flipZ)
  [25] oobMode (0=black, 1=clamp, 2=mirror)
  [26-29] gamma (float LE)
  [30-35] reserved for future expansion
```

### Appendix C: EEPROM Layout

```
Address 0x000-0x0FF: Config Slot 0
Address 0x100-0x1FF: Config Slot 1
Address 0x200-0x2FF: Config Slot 2
Address 0x300-0x3FF: Config Slot 3
Address 0x400-0x4FF: Active Slot Pointer
Address 0x500-0xFFF: Reserved
```

**Wear Leveling:** Rotate writes across 4 slots to extend EEPROM life.

### Appendix D: Performance Benchmarks

**Target vs Actual (to be measured during Phase 1):**

| Operation | Target | Current | Phase 1 | Phase 5 |
|-----------|--------|---------|---------|---------|
| Frame decode (256px RGB332) | <10ms | TBD | TBD | TBD |
| Frame decode (256px RGB565) | <20ms | TBD | TBD | TBD |
| Single pixel extraction | <1ms | TBD | TBD | TBD |
| Full render | <15ms | TBD | TBD | TBD |
| Memory overhead | <5KB | TBD | TBD | TBD |
| Discovery response | <5ms | TBD | TBD | TBD |

---

**Document Control:**
- Author: FataMorgana Development Team
- Reviewers: TBD
- Approval: TBD
- Next Review: End of Phase 1
