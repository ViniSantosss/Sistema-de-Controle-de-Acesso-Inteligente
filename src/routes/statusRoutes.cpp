#include "routes/statusRoutes.h"

#include <ArduinoJson.h>
#include <WiFi.h>

void configurarRotasStatus(WebServer& server) {
    server.on("/api/status", HTTP_GET, [&server]() {
        JsonDocument documento;
        documento["status"] = "online";
        documento["wifi"] = WiFi.status() == WL_CONNECTED;
        documento["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
        documento["ip"] = WiFi.localIP().toString();

        String json;
        serializeJson(documento, json);

        server.send(200, "application/json", json);
    });
}