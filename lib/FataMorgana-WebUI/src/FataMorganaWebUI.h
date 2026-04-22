/**
 * @file FataMorganaWebUI.h
 * @brief Optional Web Interface Component for FataMorgana
 *
 * This component provides a web-based configuration interface with
 * WebSocket real-time updates. It's optional and only needed if you
 * want remote configuration capabilities.
 */

#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "FataMorganaClient.h"

/**
 * @brief Web interface and WebSocket server for FataMorgana
 *
 * Provides HTTP endpoints for configuration and status, plus
 * WebSocket for real-time updates. This is an optional component.
 */
class FataMorganaWebUI {
public:
    /**
     * Constructor
     * @param client Reference to FataMorganaClient instance
     */
    FataMorganaWebUI(FataMorganaClient& client);

    /**
     * Initialize web interface (call in setup())
     * @param httpPort HTTP server port (default: 80)
     * @param wsPort WebSocket server port (default: 81)
     * @return true if initialization successful
     */
    bool begin(uint16_t httpPort = 80, uint16_t wsPort = 81);

    /**
     * Process web requests and WebSocket messages (call in loop())
     */
    void loop();

    /**
     * Enable/disable WebSocket broadcasting
     * @param enable true to enable WebSocket updates
     */
    void enableWebSocket(bool enable = true);

    /**
     * Broadcast status update to all WebSocket clients
     */
    void broadcastStatus();

private:
    FataMorganaClient& _client;
    ESP8266WebServer* _webServer;
    WebSocketsServer* _wsServer;
    bool _wsEnabled;
    unsigned long _lastBroadcast;
    uint16_t _httpPort;
    uint16_t _wsPort;

    // HTTP request handlers
    void handleRoot();
    void handleStatus();
    void handleGetConfig();
    void handleSetConfig();

    // WebSocket event handler
    static void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static FataMorganaWebUI* _instance;  // For static callback

    // JSON builders
    String buildStatusJson();
    String buildConfigJson();
};
