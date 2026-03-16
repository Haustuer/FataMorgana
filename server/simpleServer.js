const express = require("express");
const dgram = require("dgram");
const app = express();
const udp = dgram.createSocket("udp4");

const UDP_PORT = 7777;
const BROADCAST_ADDR = "255.255.255.255";
const LED_COUNT = 100;

app.use(express.static("public"));
app.use(express.json());

udp.bind(() => {
  udp.setBroadcast(true);
});

app.post("/color", (req, res) => {
  const { r, g, b } = req.body;

  // Build UDP packet
  const packet = Buffer.alloc(2 + LED_COUNT * 3);
  packet[0] = 0xAA;           // start byte
  packet[1] = LED_COUNT;      // number of LEDs

  for (let i = 0; i < LED_COUNT; i++) {
    packet[2 + i * 3 + 0] = r;
    packet[2 + i * 3 + 1] = g;
    packet[2 + i * 3 + 2] = b;
  }

  udp.send(packet, 0, packet.length, UDP_PORT, BROADCAST_ADDR);
  res.json({ ok: true });
});

app.listen(3000, () => console.log("Web UI on http://localhost:3000"));