/**
 * @file FataMorganaWebUI.cpp
 * @brief Implementation of FataMorgana Web Interface
 */

#include "FataMorganaWebUI.h"
#include "FataMorganaWebPage.h"  // Contains HTML page

// Static instance for WebSocket callback
FataMorganaWebUI* FataMorganaWebUI::_instance = nullptr;

FataMorganaWebUI::FataMorganaWebUI(FataMorganaClient& client)
    : _client(client),
      _webServer(nullptr),
      _wsServer(nullptr),
      _wsEnabled(true),
      _lastBroadcast(0),
      _httpPort(80),
      _wsPort(81) {
    _instance = this;
}

bool FataMorganaWebUI::begin(uint16_t httpPort, uint16_t wsPort) {
    _httpPort = httpPort;
    _wsPort = wsPort;

    // Create servers with specified ports
    _webServer = new ESP8266WebServer(_httpPort);
    _wsServer = new WebSocketsServer(_wsPort);

    // Setup HTTP routes
    _webServer->on("/", [this]() { handleRoot(); });
    _webServer->on("/status.json", [this]() { handleStatus(); });
    _webServer->on("/config.json", [this]() { handleGetConfig(); });
    _webServer->on("/config", HTTP_POST, [this]() { handleSetConfig(); });

    // Start HTTP server
    _webServer->begin();

    // Start WebSocket server
    _wsServer->begin();
    _wsServer->onEvent(webSocketEvent);

    Serial.printf("FataMorgana WebUI: HTTP on port %u\n", _httpPort);
    Serial.printf("FataMorgana WebUI: WebSocket on port %u\n", _wsPort);
    Serial.printf("FataMorgana WebUI: http://%s/\n", WiFi.localIP().toString().c_str());

    return true;
}

void FataMorganaWebUI::loop() {
    if (_webServer) _webServer->handleClient();
    if (_wsServer) _wsServer->loop();

    // Broadcast status updates every 200ms if enabled
    if (_wsEnabled && (millis() - _lastBroadcast >= 200)) {
        broadcastStatus();
        _lastBroadcast = millis();
    }
}

void FataMorganaWebUI::enableWebSocket(bool enable) {
    _wsEnabled = enable;
}

void FataMorganaWebUI::broadcastStatus() {
    if (_wsServer && _wsServer->connectedClients() > 0) {
        String statusJson = buildStatusJson();
        _wsServer->broadcastTXT(statusJson);
    }
}

void FataMorganaWebUI::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (!_instance) return;

    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("FataMorgana WebUI: WebSocket [%u] disconnected\n", num);
            break;

        case WStype_CONNECTED:
            {
                IPAddress ip = _instance->_wsServer->remoteIP(num);
                Serial.printf("FataMorgana WebUI: WebSocket [%u] connected from %s\n",
                              num, ip.toString().c_str());
                // Send initial status on connect
                String statusJson = _instance->buildStatusJson();
                _instance->_wsServer->sendTXT(num, statusJson);
            }
            break;

        case WStype_TEXT:
            Serial.printf("FataMorgana WebUI: WebSocket [%u] received: %s\n", num, payload);
            break;

        case WStype_BIN:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        case WStype_PING:
        case WStype_PONG:
        default:
            // Ignore other event types
            break;
    }
}

String FataMorganaWebUI::buildStatusJson() {
    JsonDocument doc;
    char mappingStr[64];
    char lastFrameStr[32];

    const FataMorganaMapping& mapping = _client.getMapping();
    fatamorgana_mappingSummary(mapping, mappingStr, sizeof(mappingStr));

    snprintf(lastFrameStr, sizeof(lastFrameStr), "frame #%lu",
             static_cast<unsigned long>(_client.getRenderedFrames()));

    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["wifiStatus"] = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
    doc["multicastGroup"] = FATAMORGANA_MULTICAST_ADDR.toString();
    doc["multicastPort"] = FATAMORGANA_MULTICAST_PORT;
    doc["responsePort"] = FATAMORGANA_RESPONSE_PORT;
    doc["ledCount"] = _client.getStrip().numPixels();
    doc["mapping"] = mappingStr;
    doc["mappingMode"] = fatamorgana_mappingModeName(mapping.mode);
    doc["sampleMode"] = fatamorgana_sampleModeName(mapping.sampleMode);
    doc["lastFrame"] = lastFrameStr;
    doc["imageSize"] = String(_client.getLastFrameWidth()) + "x" + String(_client.getLastFrameHeight());
    doc["rgbType"] = fatamorgana_rgbTypeName(_client.getLastFrameRgbType());
    doc["chunks"] = String(_client.isFrameInProgress() ? "in progress" : "complete");
    doc["packets"] = String(_client.getAcceptedPackets()) + " accepted, " +
                     String(_client.getRejectedPackets()) + " rejected";
    doc["renderedFrames"] = _client.getRenderedFrames();
    doc["lastRender"] = String(millis()) + " ms";
    doc["frameInProgress"] = _client.isFrameInProgress();

    String json;
    serializeJson(doc, json);
    return json;
}

String FataMorganaWebUI::buildConfigJson() {
    JsonDocument doc;
    const FataMorganaMapping& mapping = _client.getMapping();

    doc["mode"] = mapping.mode;
    doc["sampleMode"] = mapping.sampleMode;
    doc["rowIndex"] = mapping.rowIndex;
    doc["columnIndex"] = mapping.columnIndex;
    doc["linePixels"] = mapping.linePixels;
    doc["rectX"] = mapping.rectX;
    doc["rectY"] = mapping.rectY;
    doc["rectWidth"] = mapping.rectWidth;
    doc["rectHeight"] = mapping.rectHeight;
    doc["serpentine"] = mapping.serpentine;
    doc["rotation"] = mapping.rotation;
    doc["flipX"] = mapping.flipX;
    doc["flipY"] = mapping.flipY;
    doc["flipZ"] = mapping.flipZ;
    doc["gamma"] = mapping.gamma;
    doc["oobMode"] = mapping.oobMode;
    doc["ledCount"] = _client.getStrip().numPixels();

    String json;
    serializeJson(doc, json);
    return json;
}

void FataMorganaWebUI::handleRoot() {
    _webServer->send_P(200, PSTR("text/html"), FATAMORGANA_WEB_PAGE_HTML);
}

void FataMorganaWebUI::handleStatus() {
    _webServer->sendHeader("Cache-Control", "no-store");
    _webServer->send(200, "application/json", buildStatusJson());
}

void FataMorganaWebUI::handleGetConfig() {
    _webServer->sendHeader("Cache-Control", "no-store");
    _webServer->send(200, "application/json", buildConfigJson());
}

void FataMorganaWebUI::handleSetConfig() {
    if (!_webServer->hasArg("plain")) {
        _webServer->send(400, "application/json", "{\"ok\":false,\"error\":\"missing request body\"}");
        return;
    }

    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, _webServer->arg("plain"));
    if (error) {
        _webServer->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid json\"}");
        return;
    }

    // Apply configuration changes
    if (doc["mode"].is<uint8_t>()) {
        uint8_t mode = doc["mode"];
        if (mode == FATAMORGANA_MAPPING_ROW) {
            _client.setRowMapping(
                doc["rowIndex"] | 0,
                doc["linePixels"] | _client.getStrip().numPixels()
            );
        } else if (mode == FATAMORGANA_MAPPING_COLUMN) {
            _client.setColumnMapping(
                doc["columnIndex"] | 0,
                doc["linePixels"] | _client.getStrip().numPixels()
            );
        } else if (mode == FATAMORGANA_MAPPING_RECTANGLE) {
            _client.setRectangle(
                doc["rectX"] | 0,
                doc["rectY"] | 0,
                doc["rectWidth"] | 10,
                doc["rectHeight"] | 10
            );
        }
    }

    if (doc["sampleMode"].is<uint8_t>()) {
        _client.setSampleMode(doc["sampleMode"]);
    }

    if (doc["serpentine"].is<uint8_t>()) {
        _client.setSerpentine(doc["serpentine"]);
    }

    if (doc["rotation"].is<uint8_t>()) {
        _client.setRotation(doc["rotation"]);
    }

    if (doc.containsKey("flipX") || doc.containsKey("flipY") || doc.containsKey("flipZ")) {
        _client.setFlip(
            doc["flipX"] | false,
            doc["flipY"] | false,
            doc["flipZ"] | false
        );
    }

    if (doc["gamma"].is<float>()) {
        _client.setGamma(doc["gamma"]);
    }

    if (doc["oobMode"].is<uint8_t>()) {
        _client.setOOBMode(doc["oobMode"]);
    }

    // Re-render last frame with new settings for immediate visual feedback
    if (_client.reRenderLastFrame()) {
        Serial.println(F("FataMorgana WebUI: Re-rendered with new settings"));
    }

    // Broadcast status update
    broadcastStatus();

    _webServer->send(200, "application/json", buildConfigJson());
}
