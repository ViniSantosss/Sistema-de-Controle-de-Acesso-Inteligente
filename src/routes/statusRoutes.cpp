#include "routes/statusRoutes.h"

#include <WiFi.h>

void configurarRotasStatus(WebServer& server) {
    server.on("/api/status", HTTP_GET, [&server]() {
        String json = "{";
        json += "\"status\":\"online\",";
        json += "\"wifi\":";
        json += (WiFi.status() == WL_CONNECTED ? "true" : "false");
        json += ",";
        json += "\"ip\":\"";
        json += WiFi.localIP().toString();
        json += "\"";
        json += "}";

        server.send(200, "application/json", json);
    });
}