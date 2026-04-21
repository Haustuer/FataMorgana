const form = document.getElementById("frameForm");
const widthInput = document.getElementById("width");
const heightInput = document.getElementById("height");
const rgbTypeSelect = document.getElementById("rgbType");
const message = document.getElementById("message");
const previewCanvas = document.getElementById("previewCanvas");
const previewContext = previewCanvas.getContext("2d");

function setText(id, value) {
  document.getElementById(id).textContent = value;
}

function quantizeRgb332(value) {
  return Math.round((Math.round((value / 255) * 7) / 7) * 255);
}

function quantizeRgb332Blue(value) {
  return Math.round((Math.round((value / 255) * 3) / 3) * 255);
}

function quantizeRgb565Channel(value, levels) {
  return Math.round((Math.round((value / 255) * levels) / levels) * 255);
}

function gradientPixel(x, y, width, height, rgbType) {
  let r = Math.round((x / Math.max(1, width - 1)) * 255);
  let g = Math.round((y / Math.max(1, height - 1)) * 255);
  let b = Math.round(((x + y) / Math.max(1, width + height - 2)) * 255);

  if (rgbType === 1) {
    r = quantizeRgb565Channel(r, 31);
    g = quantizeRgb565Channel(g, 63);
    b = quantizeRgb565Channel(b, 31);
  } else {
    r = quantizeRgb332(r);
    g = quantizeRgb332(g);
    b = quantizeRgb332Blue(b);
  }

  return [r, g, b];
}

function renderPreview() {
  const width = Math.max(1, Number(widthInput.value) || 1);
  const height = Math.max(1, Number(heightInput.value) || 1);
  const rgbType = Number(rgbTypeSelect.value || 0);

  previewCanvas.width = width;
  previewCanvas.height = height;

  const image = previewContext.createImageData(width, height);

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const [r, g, b] = gradientPixel(x, y, width, height, rgbType);
      const offset = (y * width + x) * 4;
      image.data[offset] = r;
      image.data[offset + 1] = g;
      image.data[offset + 2] = b;
      image.data[offset + 3] = 255;
    }
  }

  previewContext.putImageData(image, 0, 0);
}

function renderLastSend(lastSend) {
  setText("frameCounter", lastSend.frameCounter ?? "-");
  setText("frameSize", `${lastSend.width} x ${lastSend.height}`);
  setText("rgbTypeLabel", lastSend.rgbTypeLabel ?? "-");
  setText("chunkCount", lastSend.chunkCount ?? 0);
  setText("payloadBytes", lastSend.payloadBytes ?? 0);
  setText("packetBytes", Array.isArray(lastSend.packetBytes) ? lastSend.packetBytes.join(", ") : "-");
}

function renderConfig(config) {
  widthInput.value = config.defaults.width;
  heightInput.value = config.defaults.height;

  rgbTypeSelect.innerHTML = "";
  for (const option of config.rgbTypes) {
    const element = document.createElement("option");
    element.value = option.value;
    element.textContent = option.label;
    if (option.value === config.defaults.rgbType) {
      element.selected = true;
    }
    rgbTypeSelect.appendChild(element);
  }

  setText("httpPort", config.httpPort);
  setText("udpTarget", `${config.broadcastAddress}:${config.udpPort}`);
  setText("maxPayload", `${config.limits.maxPayloadBytesPerPacket} bytes`);
  setText("maxFrameBytes", `${config.limits.maxFrameBytes} bytes`);
  renderLastSend(config.lastSend);
  renderPreview();
}

async function loadConfig() {
  const response = await fetch("/api/config", { cache: "no-store" });
  const config = await response.json();
  renderConfig(config);
  message.textContent = "Ready. Gradient is the default test image.";
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  message.textContent = "Sending gradient frame...";

  const payload = {
    width: Number(widthInput.value),
    height: Number(heightInput.value),
    rgbType: Number(rgbTypeSelect.value),
  };

  try {
    const response = await fetch("/api/frame", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });

    const result = await response.json();
    if (!response.ok) {
      throw new Error(result.error || "Request failed");
    }

    renderLastSend(result);
    message.textContent = `Sent gradient frame ${result.frameCounter} as ${result.chunkCount} chunk(s).`;
  } catch (error) {
    message.textContent = error.message;
  }
});

widthInput.addEventListener("input", renderPreview);
heightInput.addEventListener("input", renderPreview);
rgbTypeSelect.addEventListener("change", renderPreview);

loadConfig().catch((error) => {
  message.textContent = error.message;
});
