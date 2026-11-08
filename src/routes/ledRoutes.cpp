#include <WebServer.h>

void configurarRotasLed(WebServer& server, int ledPin) {
    server.on("/api/led/on", HTTP_GET, [&server, ledPin]() {
        digitalWrite(ledPin, HIGH);

        server.send(
            200,
            "application/json",
            "{\"led\":true}"
        );
    });

    server.on("/api/led/off", HTTP_GET, [&server, ledPin]() {
        digitalWrite(ledPin, LOW);

        server.send(
            200,
            "application/json",
            "{\"led\":false}"
        );
    });
}