#pragma once

#include <pgmspace.h>

static const char kStatusPageHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FataMorgana ESP Status</title>
  <style>
    :root {
      color-scheme: dark;
      font-family: Arial, sans-serif;
    }

    body {
      margin: 0;
      padding: 20px;
      background: #111827;
      color: #f3f4f6;
    }

    h1 {
      margin-top: 0;
      font-size: 1.5rem;
    }

    p {
      color: #cbd5e1;
    }

    .grid {
      display: grid;
      gap: 12px;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    }

    .card {
      background: #1f2937;
      border: 1px solid #374151;
      border-radius: 12px;
      padding: 16px;
    }

    .label {
      font-size: 0.8rem;
      color: #93c5fd;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .value {
      margin-top: 6px;
      font-size: 1.1rem;
      font-weight: 600;
      word-break: break-word;
    }

    .small {
      font-size: 0.9rem;
      color: #cbd5e1;
    }
  </style>
</head>
<body>
  <h1>FataMorgana ESP Status</h1>
  <p>Built-in device page compiled into the firmware image.</p>

  <div class="grid">
    <div class="card"><div class="label">IP Address</div><div class="value" id="ip">-</div></div>
    <div class="card"><div class="label">WiFi Status</div><div class="value" id="wifiStatus">-</div></div>
    <div class="card"><div class="label">UDP Port</div><div class="value" id="udpPort">-</div></div>
    <div class="card"><div class="label">LED Count</div><div class="value" id="ledCount">-</div></div>
    <div class="card"><div class="label">Region</div><div class="value" id="region">-</div></div>
    <div class="card"><div class="label">Last Frame</div><div class="value" id="lastFrame">-</div></div>
    <div class="card"><div class="label">Image Size</div><div class="value" id="imageSize">-</div></div>
    <div class="card"><div class="label">RGB Type</div><div class="value" id="rgbType">-</div></div>
    <div class="card"><div class="label">Chunks</div><div class="value" id="chunks">-</div></div>
    <div class="card"><div class="label">Packets</div><div class="value" id="packets">-</div></div>
    <div class="card"><div class="label">Rendered Frames</div><div class="value" id="renderedFrames">-</div></div>
    <div class="card"><div class="label">Last Render</div><div class="value" id="lastRender">-</div></div>
  </div>

  <p class="small">This page refreshes automatically every second.</p>

  <script>
    function setText(id, value) {
      document.getElementById(id).textContent = value;
    }

    async function refreshStatus() {
      try {
        const response = await fetch('/status.json', { cache: 'no-store' });
        const status = await response.json();

        setText('ip', status.ip);
        setText('wifiStatus', status.wifiStatus);
        setText('udpPort', status.udpPort);
        setText('ledCount', status.ledCount);
        setText('region', status.region);
        setText('lastFrame', status.lastFrame);
        setText('imageSize', status.imageSize);
        setText('rgbType', status.rgbType);
        setText('chunks', status.chunks);
        setText('packets', status.packets);
        setText('renderedFrames', status.renderedFrames);
        setText('lastRender', status.lastRender);
      } catch (error) {
        setText('wifiStatus', 'status fetch failed');
      }
    }

    refreshStatus();
    setInterval(refreshStatus, 1000);
  </script>
</body>
</html>
)HTML";
