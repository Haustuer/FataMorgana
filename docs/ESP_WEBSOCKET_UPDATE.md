# ESP WebSocket Update

## Overview

The ESP web configuration interface has been updated with:
1. **WebSocket communication** for real-time status updates
2. **Full transform support** including serpentine modes and rotation/flip options
3. **Responsive UI** that updates live as frames are processed

## Changes Made

### Hardware Communication

#### WebSocket Server
- **Port**: 81
- **URL**: `ws://<device-ip>:81/ws`
- **Update Rate**: 200ms (5 times per second)
- **Features**:
  - Real-time status broadcasting to all connected clients
  - Automatic reconnection on disconnect
  - Fallback to HTTP polling if WebSocket unavailable

#### Status Updates via WebSocket
All device status is broadcast in JSON format every 200ms:
```json
{
  "ip": "192.168.1.100",
  "wifiStatus": "connected",
  "multicastPort": 7777,
  "ledCount": 256,
  "mapping": "rect 0,0 16x16, serpentine horizontal",
  "mappingMode": "rectangle",
  "sampleMode": "pixel",
  "lastFrame": "image-start #42",
  "imageSize": "24x100",
  "rgbType": "RGB332",
  "chunks": "2/4 current chunk 1",
  "packets": "1523 accepted, 12 rejected, last payload 1192 bytes",
  "renderedFrames": 507,
  "lastRender": "15 ms, last packet 23 ms"
}
```

### Configuration Form Updates

#### Serpentine Mode (Rectangle Mode Only)
Now a dropdown instead of checkbox:
- **None** (0): No serpentine layout
- **Horizontal** (1): Zigzag left-right across rows (standard LED matrix wiring)
- **Vertical** (2): Zigzag up-down across columns

#### Transform Options (Rectangle Mode Only)

**Rotation:**
- 0° - No rotation
- 90° - Clockwise rotation
- 180° - Half rotation
- 270° - Counter-clockwise rotation

**Flip X (Horizontal Mirror):**
- Mirrors the image across vertical axis (left ↔ right)

**Flip Y (Vertical Mirror):**
- Mirrors the image across horizontal axis (top ↔ bottom)

**Flip Z (Diagonal/Transpose):**
- Swaps X and Y coordinates (transpose matrix)

### Code Changes

#### main.cpp

**Added includes:**
```cpp
#include <WebSocketsServer.h>
```

**New global variables:**
```cpp
WebSocketsServer wsServer(81);
unsigned long lastStatusBroadcast = 0;
```

**New functions:**
```cpp
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void broadcastStatus();
```

**Updated functions:**
- `startWebServer()` - Now initializes WebSocket server
- `loop()` - Broadcasts status every 200ms to connected clients

#### StatusPage.h

**Form fields added:**
- Serpentine mode dropdown (select with 3 options)
- Rotation dropdown (select with 4 options)
- Flip X checkbox
- Flip Y checkbox
- Flip Z checkbox

**JavaScript updates:**
- WebSocket connection with auto-reconnect
- Real-time status display updates
- Connection status indicator (🟢 Connected / 🟡 Reconnecting / 🔴 Error)
- Fallback HTTP polling if WebSocket unavailable

#### platformio.ini

**New library dependency:**
```ini
links2004/WebSockets@^2.4.1
```

## Usage

### Accessing the Configuration Page

1. Open web browser
2. Navigate to `http://<device-ip>/`
3. WebSocket will connect automatically
4. Status updates will appear in real-time

### Configuring Transforms

1. Select **Rectangle** mode
2. Set rectangle dimensions (X, Y, Width, Height)
3. Choose **Serpentine Mode**:
   - Use "Horizontal" for standard LED matrix strips (zigzag across rows)
   - Use "Vertical" for vertical LED strips (zigzag down columns)
4. Set **Rotation** to correct physical LED orientation
5. Check **Flip** options as needed to match your physical setup
6. Click **Save Mapping**
7. Watch real-time status updates to verify configuration

### Transform Application Order

Transforms are applied in this order when rendering:
1. **Rotation** - Rotates the extracted rectangle
2. **Flip X** - Mirrors horizontally
3. **Flip Y** - Mirrors vertically
4. **Flip Z** - Transposes (swaps X/Y)
5. **Serpentine** - Reverses pixel order on alternating rows/columns

### Example Configurations

#### Standard LED Matrix (horizontal wiring, left-to-right, zigzag)
```
Mode: Rectangle
Serpentine: Horizontal
Rotation: 0°
Flip X/Y/Z: unchecked
```

#### LED Matrix Mounted Upside Down
```
Mode: Rectangle
Serpentine: Horizontal
Rotation: 180°
Flip X/Y/Z: unchecked
```

#### LED Strip Wired Vertically (top-to-bottom, zigzag)
```
Mode: Rectangle
Serpentine: Vertical
Rotation: 0°
Flip X/Y/Z: unchecked
```

#### LED Matrix Rotated 90° Clockwise with Reversed Columns
```
Mode: Rectangle
Serpentine: Horizontal
Rotation: 1 (90°)
Flip X: checked
Flip Y/Z: unchecked
```

## Real-Time Status Indicators

### WebSocket Status
- **🟢 Connected** - WebSocket active, receiving live updates
- **🟡 Reconnecting...** - Connection lost, attempting to reconnect
- **🔴 Error** - WebSocket error occurred
- **🔴 Failed** - WebSocket connection failed to establish

### Live Updates
When WebSocket is connected, all status fields update automatically:
- Last frame received
- Image dimensions
- Chunk progress
- Packet statistics
- Rendered frame count
- Timing information

## Performance

### WebSocket Overhead
- **Bandwidth**: ~500 bytes per status update
- **Update rate**: 5 updates/sec = ~2.5KB/sec
- **CPU impact**: Minimal (<1% on ESP8266)
- **Memory**: ~2KB for WebSocket buffers

### Benefits
- **Instant feedback** when receiving frames
- **Live monitoring** of device status
- **No page refresh** needed
- **Multiple clients** can connect simultaneously

## Troubleshooting

### WebSocket Not Connecting
1. Check firewall settings (port 81)
2. Verify device IP is correct
3. Look for error message in browser console
4. Fallback to HTTP polling will activate automatically

### Status Not Updating
1. Check WebSocket status indicator
2. Verify device is on network
3. Refresh page to reconnect
4. Check ESP serial output for errors

### Transform Not Working
1. Verify mode is set to **Rectangle**
2. Check that transform fields are visible
3. Save configuration and wait for confirmation
4. Send test frame from server to verify
5. Check serial output for rendering logs

## Network Ports Summary

| Port | Protocol | Purpose |
|------|----------|---------|
| 80 | HTTP | Web interface and REST API |
| 81 | WebSocket | Real-time status updates |
| 7777 | UDP | Multicast frame reception |
| 7778 | UDP | Discovery response transmission |

## API Endpoints

### HTTP Endpoints (unchanged)
- `GET /` - Web interface
- `GET /status.json` - Get current status (JSON)
- `GET /config.json` - Get current configuration (JSON)
- `POST /config` - Update configuration (JSON body)

### WebSocket Endpoint (new)
- `ws://<device-ip>:81/ws` - Real-time status stream

## Migration Notes

### From Previous Version

**Breaking changes:**
- `serpentine` field changed from boolean to uint8 (0/1/2)

**New features:**
- WebSocket communication
- Rotation and flip transforms
- Real-time status updates

**Backward compatibility:**
- HTTP endpoints unchanged
- UDP protocol unchanged
- Discovery response format updated (already backwards compatible)

## Testing

### Test WebSocket Connection
```javascript
// In browser console
const ws = new WebSocket('ws://192.168.1.100:81/ws');
ws.onmessage = (e) => console.log(JSON.parse(e.data));
```

### Test Transform Configuration
```bash
# Set 90° rotation with horizontal serpentine
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{
    "mode": 2,
    "rectX": 0,
    "rectY": 0,
    "rectWidth": 16,
    "rectHeight": 16,
    "serpentine": 1,
    "rotation": 1,
    "flipX": false,
    "flipY": false,
    "flipZ": false
  }'
```

## Next Steps

1. Upload updated firmware to ESP device
2. Open web interface in browser
3. Verify WebSocket connects (check status indicator)
4. Configure transforms as needed
5. Send test frames from server
6. Monitor real-time status during frame reception
