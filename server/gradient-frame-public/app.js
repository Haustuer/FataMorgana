const form = document.getElementById("frameForm");
const widthInput = document.getElementById("width");
const heightInput = document.getElementById("height");
const rgbTypeSelect = document.getElementById("rgbType");
const message = document.getElementById("message");

function setText(id, value) {
  document.getElementById(id).textContent = value;
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

loadConfig().catch((error) => {
  message.textContent = error.message;
});
