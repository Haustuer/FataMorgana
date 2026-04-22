# FataMorgana Network Configuration Quick Reference

## Network Addresses

| Component | Address | Port | Protocol | Direction |
|-----------|---------|------|----------|-----------|
| **Server → Devices** | `239.255.42.1` | `7777` | UDP Multicast | Outbound |
| **Devices → Server** | Server IP | `7778` | UDP Unicast | Inbound |

## Multicast Group

```
Multicast Address: 239.255.42.1
Multicast Port:    7777
TTL:              1 (local subnet only)
Address Family:    IPv4 Site-Local Multicast
```

**Why 239.255.42.1?**
- `239.0.0.0/8` - IPv4 site-local multicast range (RFC 2365)
- `239.255.x.x` - Administratively scoped
- `.42` - Just a fun choice (Hitchhiker's Guide reference)
- Unlikely to conflict with other services

## Server Configuration

### Node.js (dgram)

```javascript
const dgram = require('dgram');

const MULTICAST_ADDR = '239.255.42.1';
const MULTICAST_PORT = 7777;
const RESPONSE_PORT = 7778;

// Create socket for sending multicast
const sendSocket = dgram.createSocket({ type: 'udp4', reuseAddr: true });
sendSocket.bind(() => {
  sendSocket.setMulticastTTL(1); // Local subnet only
  console.log(`Sending to multicast ${MULTICAST_ADDR}:${MULTICAST_PORT}`);
});

// Create socket for receiving unicast responses
const receiveSocket = dgram.createSocket({ type: 'udp4', reuseAddr: true });
receiveSocket.bind(RESPONSE_PORT, () => {
  console.log(`Listening for responses on port ${RESPONSE_PORT}`);
});

receiveSocket.on('message', (msg, rinfo) => {
  console.log(`Response from ${rinfo.address}:${rinfo.port}`);
  const response = JSON.parse(msg.toString());
  // Process device response...
});

// Send frame
const packet = Buffer.from([...]); // Your frame data
sendSocket.send(packet, 0, packet.length, MULTICAST_PORT, MULTICAST_ADDR);
```

### Environment Variables

```bash
MULTICAST_ADDR=239.255.42.1
MULTICAST_PORT=7777
RESPONSE_PORT=7778
MULTICAST_TTL=1
```

## Device Configuration (ESP8266)

### Arduino/PlatformIO

```cpp
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const IPAddress MULTICAST_ADDR(239, 255, 42, 1);
const uint16_t MULTICAST_PORT = 7777;
const uint16_t RESPONSE_PORT = 7778;

WiFiUDP udp;

void setup() {
  // Connect to WiFi first...
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);

  // Join multicast group
  udp.beginMulticast(WiFi.localIP(), MULTICAST_ADDR, MULTICAST_PORT);
  Serial.printf("Joined multicast group %s:%d\n",
    MULTICAST_ADDR.toString().c_str(), MULTICAST_PORT);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    // Read packet
    uint8_t buffer[1200];
    int len = udp.read(buffer, sizeof(buffer));

    // Process frame...
    handleFrame(buffer, len);

    // If discovery request, send unicast response
    if (isDiscoveryRequest(buffer)) {
      IPAddress serverIP = extractServerIP(buffer);
      String response = buildJSONResponse();

      udp.beginPacket(serverIP, RESPONSE_PORT);
      udp.write((const uint8_t*)response.c_str(), response.length());
      udp.endPacket();
    }
  }
}
```

## Router/Switch Configuration

### Required Settings

✅ **IGMP Snooping:** Enabled (recommended)
- Optimizes multicast delivery
- Reduces unnecessary traffic
- Not strictly required

✅ **Multicast Forwarding:** Enabled
- Some routers call this "Multicast Routing"
- Or "IGMP Proxy"

✅ **Multicast on WiFi:** Enabled
- May be under "Wireless Settings"
- Or "Advanced WiFi Settings"

### Optional Settings

⚙️ **IGMP Version:** v2 or v3
- v2 is sufficient for this use case

⚙️ **Multicast Rate Limit:** Disabled
- Or set high enough for your frame rate

⚙️ **AP Isolation:** Disabled
- Must be off for multicast to work

## Firewall Configuration

### Linux (iptables)

```bash
# Allow multicast on outbound (server)
sudo iptables -A OUTPUT -p udp -d 239.255.42.1 --dport 7777 -j ACCEPT

# Allow unicast responses inbound (server)
sudo iptables -A INPUT -p udp --dport 7778 -j ACCEPT

# Allow multicast on inbound (devices)
sudo iptables -A INPUT -p udp -d 239.255.42.1 --dport 7777 -j ACCEPT

# Allow unicast on outbound (devices)
sudo iptables -A OUTPUT -p udp --dport 7778 -j ACCEPT
```

### Windows Firewall

```powershell
# Allow multicast (server sending)
New-NetFirewallRule -DisplayName "FataMorgana Multicast Out" `
  -Direction Outbound -Protocol UDP -RemotePort 7777 `
  -RemoteAddress 239.255.42.1 -Action Allow

# Allow unicast responses (server receiving)
New-NetFirewallRule -DisplayName "FataMorgana Unicast In" `
  -Direction Inbound -Protocol UDP -LocalPort 7778 -Action Allow
```

### macOS (pf)

```bash
# Add to /etc/pf.conf
pass out proto udp from any to 239.255.42.1 port 7777
pass in proto udp from any to any port 7778
```

## Testing Multicast

### Test Multicast Reachability

```bash
# On server machine
ping 239.255.42.1

# Should see responses if devices are listening
# (ESP8266 may not respond to ICMP, but test still validates routing)
```

### Monitor Multicast Traffic

```bash
# Linux/macOS
sudo tcpdump -i any 'udp and (dst 239.255.42.1 or port 7778)' -X

# Wireshark filter
udp.port == 7777 or udp.port == 7778
```

### Send Test Packet (socat)

```bash
# Send test multicast packet
echo "Hello Multicast" | socat - UDP4-DATAGRAM:239.255.42.1:7777

# Listen for responses
socat UDP4-RECV:7778 -
```

### Verify Multicast Membership (Linux)

```bash
# Show multicast groups
netstat -g

# Should show interface joined to 239.255.42.1
```

## Troubleshooting

### Devices Not Receiving Frames

1. **Check multicast group joined:**
   ```cpp
   Serial.println(WiFi.localIP());
   Serial.println(MULTICAST_ADDR);
   ```

2. **Verify router allows multicast:**
   - Check IGMP snooping enabled
   - Check multicast forwarding enabled
   - Check AP isolation disabled

3. **Test with broadcast first:**
   - Temporarily use 255.255.255.255 instead
   - If broadcast works, it's a multicast config issue

4. **Check firewall:**
   - Disable firewall temporarily to test
   - Add exceptions if that fixes it

### Server Not Receiving Responses

1. **Check listening on correct port:**
   ```javascript
   console.log(`Listening on port ${RESPONSE_PORT}`);
   ```

2. **Verify unicast routing:**
   ```bash
   ping [device IP]  # Should work
   ```

3. **Check firewall allows inbound 7778:**
   ```bash
   sudo lsof -i :7778  # Should show your server process
   ```

4. **Monitor with tcpdump:**
   ```bash
   sudo tcpdump -i any 'udp port 7778' -A
   ```

### Multicast Not Working on WiFi

Some WiFi routers drop multicast by default:

**Option 1: Enable in router settings**
- Look for "IGMP Proxy"
- Or "Multicast Routing"
- Or "Multicast Enhancement"

**Option 2: Fall back to broadcast**
```javascript
// Server: use broadcast instead
const BROADCAST_ADDR = '255.255.255.255';
sendSocket.setBroadcast(true);
sendSocket.send(packet, 0, packet.length, 7777, BROADCAST_ADDR);

// Device: use regular UDP instead of multicast
udp.begin(7777);  // Regular UDP, not beginMulticast
```

**Option 3: Use wired Ethernet**
- Multicast much more reliable on wired networks
- Connect server via Ethernet
- WiFi only for ESP devices

## Performance Tuning

### Multicast TTL

```javascript
// Default: 1 (local subnet only)
sendSocket.setMulticastTTL(1);

// Increase to cross routers (usually not needed)
sendSocket.setMulticastTTL(2);
```

### Socket Buffer Size

```cpp
// ESP8266: Increase WiFi RX buffer if dropping packets
WiFi.setRxBufferSize(2048);  // Default is ~1500
```

### IGMP Query Interval

Some routers send IGMP queries too frequently:
- Default: 125 seconds
- If getting disconnects, check router IGMP settings
- Increase query interval to 240+ seconds

## Security Notes

### Multicast Considerations

⚠️ **No authentication** - Any device on network can:
- Receive all frames (image data visible)
- Send fake discovery responses
- Inject malicious frames

⚠️ **Subnet scope only** - With TTL=1:
- Multicast doesn't leave local subnet
- Can't cross routers (by design)
- Provides some isolation

### Future Security

Consider adding:
- Pre-shared key (PSK) in config frame
- HMAC signature on packets
- AES encryption for image data

## Quick Reference Card

```
╔══════════════════════════════════════════════════════════╗
║           FataMorgana Network Configuration              ║
╠══════════════════════════════════════════════════════════╣
║ Multicast Group:  239.255.42.1:7777                     ║
║ Response Port:    7778 (unicast)                         ║
║ TTL:             1 (local subnet)                        ║
║                                                          ║
║ Server sends:     Multicast → All devices               ║
║ Devices respond:  Unicast → Server IP                   ║
║                                                          ║
║ Requirements:                                            ║
║ • IGMP Snooping enabled (recommended)                    ║
║ • Multicast forwarding enabled                           ║
║ • Firewall allows UDP 7777/7778                          ║
║ • AP isolation disabled                                  ║
╚══════════════════════════════════════════════════════════╝
```
