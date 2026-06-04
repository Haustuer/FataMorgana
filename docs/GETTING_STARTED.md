# Getting Started with FataMorgana

Complete guide to setting up your FataMorgana distributed LED display system.

## Overview

FataMorgana allows you to control multiple LED strips/matrices from a central server using UDP multicast. Each ESP device extracts its portion of the image and displays it on its LEDs.

## What You'll Need

### Hardware

**Per Device:**
- ESP8266 or ESP32 board
- WS2812B/NeoPixel LED strip or matrix
- 5V power supply (adequate for your LED count)
- Optional: Level shifter for data line

**Network:**
- WiFi router with multicast support
- Firewall configured for UDP ports 7777-7778

### Software

- PlatformIO or Arduino IDE
- FataMorgana library
- Optional: FataMorgana-WebUI library

## Step 1: Hardware Setup

### Wiring

```
ESP8266/ESP32          LED Strip
--------------        -----------
GPIO 12 ---------->   DIN (Data)
GND   -------------->  GND
5V (from PSU) ------>  5V
```

**Important:**
- Use external 5V power supply for LEDs (not ESP regulator!)
- Connect all grounds together (ESP + PSU + LEDs)
- Consider level shifter for 3.3V → 5V data signal
- Add 470Ω resistor on data line
- Add 1000µF capacitor across LED power supply

### LED Wiring Patterns

**Linear Strip:**
```
0 → 1 → 2 → 3 → 4 → ... → N
```

**Horizontal Serpentine Matrix:**
```
0  → 1  → 2  → 3
7  ← 6  ← 5  ← 4
8  → 9  → 10 → 11
15 ← 14 ← 13 ← 12
```
Use: `client.setSerpentine(SERPENTINE_HORIZONTAL)`

**Vertical Serpentine Matrix:**
```
0   8   16  24
↓   ↑   ↓   ↑
1   9   17  25
↓   ↑   ↓   ↑
2   10  18  26
↓   ↑   ↓   ↑
...
```
Use: `client.setSerpentine(SERPENTINE_VERTICAL)`

## Step 2: Install Libraries

### PlatformIO

Create `platformio.ini`:

```ini
[env:d1_mini]
platform = espressif8266
board = d1_mini
framework = arduino

lib_deps =
    FataMorgana           # Core protocol
    FataMorgana-WebUI     # Optional web interface
```

### Arduino IDE

1. **Sketch → Include Library → Manage Libraries**
2. Search for "FataMorgana"
3. Install **FataMorgana**
4. Optional: Install **FataMorgana-WebUI**

## Step 3: Write Your First Sketch

### Minimal Example

```cpp
#include <ESP8266WiFi.h>
#include <FataMorgana.h>

const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

FataMorganaClient client(256, 12);  // 256 LEDs on GPIO 12

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.begin();
  client.setRectangle(0, 0, 16, 16);  // Extract 16x16 region

  Serial.print("Ready! IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  client.loop();
}
```

### With Web Interface

```cpp
#include <ESP8266WiFi.h>
#include <FataMorgana.h>
#include <FataMorganaWebUI.h>

const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

FataMorganaClient client(256, 12);
FataMorganaWebUI webUI(client);

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.begin();
  webUI.begin();  // Start web server

  Serial.print("Web UI: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  client.loop();
  webUI.loop();
}
```

## Step 4: Upload and Test

1. **Connect ESP via USB**
2. **Upload sketch**
3. **Open Serial Monitor** (115200 baud)
4. **Note the IP address**

Example output:
```
Connecting to WiFi....
Connected! IP: 192.168.1.100
FataMorgana: Joined multicast 239.255.42.1:7777
FataMorgana: 256 LEDs on pin 12
Ready!
```

## Step 5: Set Up Server

The server sends image frames to all devices via multicast.

### Using Node.js Server (Included)

```bash
cd server
npm install
npm start
```

Access server interface: `http://localhost:3001`

### Send Test Frame

```bash
curl -X POST http://localhost:3001/api/frame \
  -H "Content-Type: application/json" \
  -d '{"width":24,"height":100,"pattern":"gradient"}'
```

You should see LEDs light up!

## Step 6: Configure Device

### Via Web Interface (if using WebUI)

1. Open `http://192.168.1.100` in browser
2. Select **Rectangle** mode
3. Set position and size
4. Choose serpentine mode if needed
5. Changes auto-save

### Via Serial Commands (Custom)

Implement your own serial config handler:

```cpp
if (Serial.available()) {
  String cmd = Serial.readStringUntil('\n');
  if (cmd.startsWith("rect ")) {
    int x, y, w, h;
    sscanf(cmd.c_str(), "rect %d %d %d %d", &x, &y, &w, &h);
    client.setRectangle(x, y, w, h);
  }
}
```

## Step 7: Multiple Devices

### Planning Your Layout

Example: 3 devices, 24x100 image

**Device 1:** Left third
```cpp
client.setRectangle(0, 0, 8, 100);
```

**Device 2:** Middle third
```cpp
client.setRectangle(8, 0, 8, 100);
```

**Device 3:** Right third
```cpp
client.setRectangle(16, 0, 8, 100);
```

### Discovery

Server can auto-discover devices:

```bash
curl -X POST http://localhost:3001/api/discover
```

View discovered devices:
```bash
curl http://localhost:3001/api/devices
```

## Troubleshooting

### LEDs Not Lighting Up

**Check:**
- ✅ ESP connected to WiFi (check Serial Monitor)
- ✅ Multicast group joined successfully
- ✅ Server sending frames (check server logs)
- ✅ Firewall allows UDP 7777
- ✅ Router supports multicast
- ✅ LED power supply adequate
- ✅ LED data pin correct (GPIO 12)

**Debug:**
```cpp
void loop() {
  client.loop();

  // Print stats every 5 seconds
  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("Frames: %lu, Packets: %lu/%lu\n",
                  client.getRenderedFrames(),
                  client.getAcceptedPackets(),
                  client.getRejectedPackets());
  }
}
```

### Multicast Not Working

**Windows:**
```bash
# Check multicast route
route print
# Should show 239.0.0.0 route
```

**Linux:**
```bash
# Add multicast route
sudo ip route add 239.0.0.0/8 dev eth0
```

**Router:**
- Enable IGMP Snooping
- Allow multicast forwarding
- Check VLAN configuration

### Web Interface Not Accessible

**Check:**
- ✅ Device IP correct
- ✅ Port 80 not blocked
- ✅ ESP on same network
- ✅ WebUI library installed

**Try:**
```cpp
webUI.begin(8080, 8081);  // Use different ports
```

### Wrong Colors/Pattern

**Check:**
- LED type matches code: `NEO_GRB` vs `NEO_RGB`
- Serpentine mode matches physical wiring
- Rectangle position/size correct
- Rotation/flips not accidentally set

## Next Steps

**Learn More:**
- Read [API.md](API.md) for complete API reference
- Try examples in `examples/` directory
- Check [TRANSFORM_FEATURE.md](TRANSFORM_FEATURE.md) for advanced transforms
- Review [FATAMORGANA_PROTOCOL.md](FATAMORGANA_PROTOCOL.md) for protocol details

**Advanced Topics:**
- Custom patterns and effects
- Integrating with other systems (MQTT, Home Assistant, etc.)
- Multiple frame sources
- Performance optimization

## Support

**Issues:**
- GitHub: https://github.com/yourname/FataMorgana/issues

**Community:**
- Share your builds!
- Contribute examples
- Report bugs

Happy building! 🎨✨
