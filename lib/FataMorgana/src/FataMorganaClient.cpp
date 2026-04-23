/**
 * @file FataMorganaClient.cpp
 * @brief Implementation of FataMorgana Client
 */

#include "FataMorganaClient.h"

FataMorganaClient::FataMorganaClient(uint16_t ledCount, uint8_t ledPin, uint8_t ledType)
    : _strip(ledCount, ledPin, ledType),
      _renderer(_strip),
      _ledCount(ledCount),
      _ledPin(ledPin),
      _frameInProgress(false),
      _activeFrameCounter(0),
      _activeRgbType(FATAMORGANA_RGB332),
      _activeWidth(0),
      _activeHeight(0),
      _activeFrameBytes(0),
      _expectedChunkCount(0),
      _receivedChunkCount(0),
      _storedFrameSize(0),
      _storedFrameWidth(0),
      _storedFrameHeight(0),
      _storedFrameRgbType(FATAMORGANA_RGB332),
      _hasStoredFrame(false),
      _acceptedPackets(0),
      _rejectedPackets(0),
      _renderedFrames(0),
      _lastFrameWidth(0),
      _lastFrameHeight(0),
      _lastFrameRgbType(FATAMORGANA_RGB332),
      _lastPacketMillis(0),
      _lastRenderMillis(0) {

    // Allocate frame buffer and chunk tracking
    _frameBuffer = new uint8_t[FATAMORGANA_MAX_FRAME_BYTES];
    _chunkReceived = new bool[FATAMORGANA_MAX_CHUNKS];
    memset(_frameBuffer, 0, FATAMORGANA_MAX_FRAME_BYTES);
    memset(_chunkReceived, 0, FATAMORGANA_MAX_CHUNKS);
    // Allocate stored frame buffer for re-rendering
    _storedFrameBuffer = new uint8_t[FATAMORGANA_MAX_FRAME_BYTES];
    memset(_storedFrameBuffer, 0, FATAMORGANA_MAX_FRAME_BYTES);

    // Set default LED count for line pixels
    _mapping.linePixels = ledCount;
}

FataMorganaClient::~FataMorganaClient() {
    delete[] _frameBuffer;
    delete[] _chunkReceived;
    delete[] _storedFrameBuffer;
}

bool FataMorganaClient::begin() {
    // Initialize LED strip
    _strip.begin();
    _strip.show();
    _strip.setBrightness(80);

    // Sanitize configuration
    fatamorgana_sanitizeMapping(_mapping, _ledCount);

    // Join multicast group
    if (!_udp.beginMulticast(WiFi.localIP(), FATAMORGANA_MULTICAST_ADDR, FATAMORGANA_MULTICAST_PORT)) {
        Serial.println(F("FataMorgana: Failed to join multicast group"));
        return false;
    }

    Serial.printf("FataMorgana: Joined multicast %s:%u\n",
                  FATAMORGANA_MULTICAST_ADDR.toString().c_str(),
                  FATAMORGANA_MULTICAST_PORT);
    Serial.printf("FataMorgana: %u LEDs on pin %u\n", _ledCount, _ledPin);

    return true;
}

void FataMorganaClient::loop() {
    const int packetSize = _udp.parsePacket();
    if (packetSize <= 0) {
        return;
    }

    if (packetSize > static_cast<int>(sizeof(_packetBuffer))) {
        _rejectedPackets++;
        return;
    }

    const int length = _udp.read(_packetBuffer, sizeof(_packetBuffer));
    if (length > 0) {
        handlePacket(_packetBuffer, static_cast<size_t>(length));
    }
}

void FataMorganaClient::handlePacket(const uint8_t* data, size_t length) {
    if (length < FATAMORGANA_HEADER_SIZE) {
        _rejectedPackets++;
        return;
    }

    // Parse header
    const uint8_t frameType = data[0];
    const uint8_t frameCounter = data[1];
    const uint8_t chunkIndex = data[2];
    const uint8_t typeData = data[3];
    const uint16_t width = static_cast<uint16_t>(data[4]) | (static_cast<uint16_t>(data[5]) << 8);
    const uint16_t height = static_cast<uint16_t>(data[6]) | (static_cast<uint16_t>(data[7]) << 8);
    const uint8_t* payload = data + FATAMORGANA_HEADER_SIZE;
    const size_t payloadLength = length - FATAMORGANA_HEADER_SIZE;

    _lastPacketMillis = millis();

    // Handle config frames
    if (frameType == FATAMORGANA_FRAME_TYPE_CONFIG) {
        const uint8_t subType = typeData;

        if (subType == FATAMORGANA_CONFIG_SUBTYPE_DISCOVERY) {
            // Discovery request: payload contains server IP (4 bytes) + port (2 bytes)
            if (payloadLength >= 6) {
                IPAddress serverIP(payload[0], payload[1], payload[2], payload[3]);
                uint16_t serverPort = (static_cast<uint16_t>(payload[4]) << 8) | payload[5];
                handleDiscoveryRequest(serverIP, serverPort);
                _acceptedPackets++;
            } else {
                _rejectedPackets++;
            }
        } else {
            _rejectedPackets++;
        }
        return;
    }

    // Handle image frames
    const uint8_t rgbType = typeData;
    _lastFrameWidth = width;
    _lastFrameHeight = height;
    _lastFrameRgbType = rgbType;

    if (frameType == FATAMORGANA_FRAME_TYPE_IMAGE_START) {
        if (chunkIndex != 0 || !beginFrame(frameCounter, rgbType, width, height)) {
            _rejectedPackets++;
            return;
        }
    } else if (frameType == FATAMORGANA_FRAME_TYPE_IMAGE_CONTINUATION) {
        if (!validateActiveFrame(frameCounter, rgbType, width, height)) {
            _rejectedPackets++;
            return;
        }
    } else {
        _rejectedPackets++;
        return;
    }

    if (!storeChunk(chunkIndex, payload, payloadLength)) {
        _rejectedPackets++;
        return;
    }

    _acceptedPackets++;

    // Check if frame is complete
    if (_receivedChunkCount == _expectedChunkCount) {
        renderCompleteFrame();
    }
}

void FataMorganaClient::handleDiscoveryRequest(IPAddress serverIP, uint16_t serverPort) {
    Serial.printf("FataMorgana: Discovery from %s:%u\n",
                  serverIP.toString().c_str(),
                  serverPort);
    sendDiscoveryResponse(serverIP, serverPort);
}

void FataMorganaClient::sendDiscoveryResponse(IPAddress serverIP, uint16_t serverPort) {
    uint8_t response[FATAMORGANA_DISCOVERY_RESPONSE_SIZE];
    memset(response, 0, sizeof(response));

    // Magic bytes "FATA"
    response[0] = 0x46;
    response[1] = 0x41;
    response[2] = 0x54;
    response[3] = 0x41;

    // Protocol version and type
    response[4] = FATAMORGANA_PROTOCOL_VERSION;
    response[5] = FATAMORGANA_RESPONSE_TYPE_DISCOVERY;
    response[6] = 0;
    response[7] = 0;

    // Device IP (network byte order - big-endian)
    IPAddress localIP = WiFi.localIP();
    response[8] = localIP[0];
    response[9] = localIP[1];
    response[10] = localIP[2];
    response[11] = localIP[3];

    // MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    memcpy(response + 12, mac, 6);

    // Chip ID (little-endian)
    uint32_t chipId = ESP.getChipId();
    response[18] = chipId & 0xFF;
    response[19] = (chipId >> 8) & 0xFF;
    response[20] = (chipId >> 16) & 0xFF;
    response[21] = (chipId >> 24) & 0xFF;

    // Uptime (little-endian)
    uint32_t uptime = millis();
    response[22] = uptime & 0xFF;
    response[23] = (uptime >> 8) & 0xFF;
    response[24] = (uptime >> 16) & 0xFF;
    response[25] = (uptime >> 24) & 0xFF;

    // LED hardware info
    response[26] = _ledCount & 0xFF;
    response[27] = (_ledCount >> 8) & 0xFF;
    response[28] = _ledPin;
    response[29] = _strip.getBrightness();
    response[30] = FATAMORGANA_VERSION_MAJOR;
    response[31] = FATAMORGANA_VERSION_MINOR;

    // Mapping configuration
    response[32] = _mapping.mode;
    response[33] = _mapping.sampleMode;
    response[34] = _mapping.rowIndex & 0xFF;
    response[35] = (_mapping.rowIndex >> 8) & 0xFF;
    response[36] = _mapping.linePixels & 0xFF;
    response[37] = (_mapping.linePixels >> 8) & 0xFF;
    response[38] = _mapping.rectX & 0xFF;
    response[39] = (_mapping.rectX >> 8) & 0xFF;
    response[40] = _mapping.rectY & 0xFF;
    response[41] = (_mapping.rectY >> 8) & 0xFF;
    response[42] = _mapping.rectWidth & 0xFF;
    response[43] = (_mapping.rectWidth >> 8) & 0xFF;
    response[44] = _mapping.rectHeight & 0xFF;
    response[45] = (_mapping.rectHeight >> 8) & 0xFF;
    response[46] = _mapping.serpentine;

    // Transform byte
    response[47] = (_mapping.rotation & 0x03) |
                   (_mapping.flipX ? 0x04 : 0) |
                   (_mapping.flipY ? 0x08 : 0) |
                   (_mapping.flipZ ? 0x10 : 0);

    // Statistics
    response[48] = _acceptedPackets & 0xFF;
    response[49] = (_acceptedPackets >> 8) & 0xFF;
    response[50] = (_acceptedPackets >> 16) & 0xFF;
    response[51] = (_acceptedPackets >> 24) & 0xFF;
    response[52] = _rejectedPackets & 0xFF;
    response[53] = (_rejectedPackets >> 8) & 0xFF;
    response[54] = (_rejectedPackets >> 16) & 0xFF;
    response[55] = (_rejectedPackets >> 24) & 0xFF;
    response[56] = _renderedFrames & 0xFF;
    response[57] = (_renderedFrames >> 8) & 0xFF;
    response[58] = (_renderedFrames >> 16) & 0xFF;
    response[59] = (_renderedFrames >> 24) & 0xFF;
    response[60] = _lastFrameWidth & 0xFF;
    response[61] = (_lastFrameWidth >> 8) & 0xFF;
    response[62] = _lastFrameHeight & 0xFF;
    response[63] = (_lastFrameHeight >> 8) & 0xFF;

    // Gamma correction (float, little-endian)
    float gamma = _mapping.gamma;
    memcpy(response + 64, &gamma, sizeof(float));

    // Send UDP packet
    _udp.beginPacket(serverIP, serverPort);
    _udp.write(response, sizeof(response));
    _udp.endPacket();

    Serial.printf("FataMorgana: Sent discovery response (%u bytes)\n", sizeof(response));
}

bool FataMorganaClient::beginFrame(uint8_t frameCounter, uint8_t rgbType, uint16_t width, uint16_t height) {
    const size_t frameBytes = fatamorgana_frameSize(width, height, rgbType);
    const uint8_t chunkCount = fatamorgana_chunkCount(frameBytes);

    if (frameBytes == 0 || frameBytes > FATAMORGANA_MAX_FRAME_BYTES ||
        chunkCount == 0 || chunkCount > FATAMORGANA_MAX_CHUNKS) {
        return false;
    }

    _activeFrameCounter = frameCounter;
    _activeRgbType = rgbType;
    _activeWidth = width;
    _activeHeight = height;
    _activeFrameBytes = frameBytes;
    _expectedChunkCount = chunkCount;
    _receivedChunkCount = 0;
    _frameInProgress = true;

    memset(_frameBuffer, 0, FATAMORGANA_MAX_FRAME_BYTES);
    memset(_chunkReceived, 0, FATAMORGANA_MAX_CHUNKS);

    return true;
}

bool FataMorganaClient::validateActiveFrame(uint8_t frameCounter, uint8_t rgbType, uint16_t width, uint16_t height) {
    return _frameInProgress &&
           frameCounter == _activeFrameCounter &&
           rgbType == _activeRgbType &&
           width == _activeWidth &&
           height == _activeHeight;
}

bool FataMorganaClient::storeChunk(uint8_t chunkIndex, const uint8_t* payload, size_t payloadLength) {
    if (!_frameInProgress || chunkIndex >= _expectedChunkCount) {
        return false;
    }

    const size_t offset = static_cast<size_t>(chunkIndex) * FATAMORGANA_MAX_PAYLOAD_SIZE;
    const size_t remaining = _activeFrameBytes - offset;
    const size_t expectedLength = remaining < FATAMORGANA_MAX_PAYLOAD_SIZE ? remaining : FATAMORGANA_MAX_PAYLOAD_SIZE;

    if (payloadLength != expectedLength) {
        return false;
    }

    memcpy(_frameBuffer + offset, payload, payloadLength);

    if (!_chunkReceived[chunkIndex]) {
        _chunkReceived[chunkIndex] = true;
        _receivedChunkCount++;
    }

    return true;
}

void FataMorganaClient::renderCompleteFrame() {
    Serial.printf("FataMorgana: Rendering frame #%u (%ux%u, %s)\n",
                  _activeFrameCounter,
                  _activeWidth,
                  _activeHeight,
                  fatamorgana_rgbTypeName(_activeRgbType));

    _renderer.renderFrame(_frameBuffer, _activeWidth, _activeHeight, _activeRgbType, _mapping);

    _renderedFrames++;
    _lastRenderMillis = millis();
    // Store frame for re-rendering
    if (_activeFrameBytes <= FATAMORGANA_MAX_FRAME_BYTES) {
        memcpy(_storedFrameBuffer, _frameBuffer, _activeFrameBytes);
        _storedFrameSize = _activeFrameBytes;
        _storedFrameWidth = _activeWidth;
        _storedFrameHeight = _activeHeight;
        _storedFrameRgbType = _activeRgbType;
        _hasStoredFrame = true;
    }

    _frameInProgress = false;
}

// Configuration methods

void FataMorganaClient::setRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    _mapping.mode = FATAMORGANA_MAPPING_RECTANGLE;
    _mapping.rectX = x;
    _mapping.rectY = y;
    _mapping.rectWidth = width;
    _mapping.rectHeight = height;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

void FataMorganaClient::setRowMapping(uint16_t rowIndex, uint16_t pixels) {
    _mapping.mode = FATAMORGANA_MAPPING_ROW;
    _mapping.rowIndex = rowIndex;
    _mapping.linePixels = pixels;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

void FataMorganaClient::setColumnMapping(uint16_t columnIndex, uint16_t pixels) {
    _mapping.mode = FATAMORGANA_MAPPING_COLUMN;
    _mapping.columnIndex = columnIndex;
    _mapping.linePixels = pixels;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

void FataMorganaClient::setSampleMode(uint8_t mode) {
    _mapping.sampleMode = mode;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

void FataMorganaClient::setSerpentine(uint8_t mode) {
    _mapping.serpentine = mode;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

void FataMorganaClient::setRotation(uint8_t rotation) {
    _mapping.rotation = rotation;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

void FataMorganaClient::setFlip(bool x, bool y, bool z) {
    _mapping.flipX = x;
    _mapping.flipY = y;
    _mapping.flipZ = z;
}

void FataMorganaClient::setBrightness(uint8_t brightness) {
    _strip.setBrightness(brightness);
}

void FataMorganaClient::setGamma(float gamma) {
    _mapping.gamma = gamma;
    fatamorgana_sanitizeMapping(_mapping, _ledCount);
}

bool FataMorganaClient::reRenderLastFrame() {
    if (!_hasStoredFrame) {
        return false;  // No frame stored yet
    }

    // Temporarily use stored frame
    uint8_t* originalBuffer = _frameBuffer;
    uint16_t originalWidth = _activeWidth;
    uint16_t originalHeight = _activeHeight;
    uint8_t originalRgbType = _activeRgbType;

    _frameBuffer = _storedFrameBuffer;
    _activeWidth = _storedFrameWidth;
    _activeHeight = _storedFrameHeight;
    _activeRgbType = _storedFrameRgbType;

    // Render with current mapping/transform settings
    _renderer.renderFrame(_frameBuffer, _activeWidth, _activeHeight, _activeRgbType, _mapping);

    // Restore original pointers
    _frameBuffer = originalBuffer;
    _activeWidth = originalWidth;
    _activeHeight = originalHeight;
    _activeRgbType = originalRgbType;

    return true;
}
