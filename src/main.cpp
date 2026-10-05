#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

const int LED_PIN = 2;
const char* ssid = "sua_rede_wifi";
const char* password = "sua_senha_wifi";

WebServer server(80);

void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("================================");
    Serial.println("Sistema de Controle de Acesso");
    Serial.println("ESP32 iniciado!");
    Serial.println("================================");

    Serial.print("Conectando ao Wi-Fi");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi conectado!");
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());

    if (!LittleFS.begin(true)) {
        Serial.println("Erro ao iniciar o LittleFS!");
        return;
    }

    Serial.println("LittleFS iniciado!");

    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");

        if (!file) {
            server.send(500, "text/plain", "Erro ao abrir index.html");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/css/style.css", HTTP_GET, []() {
        File file = LittleFS.open("/css/style.css", "r");

        if (!file) {
            server.send(404, "text/plain", "CSS nao encontrado");
            return;
        }

        server.streamFile(file, "text/css");
        file.close();
    });

    server.on("/js/app.js", HTTP_GET, []() {
        File file = LittleFS.open("/js/app.js", "r");

        if (!file) {
            server.send(404, "text/plain", "JavaScript nao encontrado");
            return;
        }

        server.streamFile(file, "application/javascript");
        file.close();
    });

    server.on("/api/status", HTTP_GET, []() {
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

    server.on("/api/led/on", HTTP_GET, []() {
        digitalWrite(LED_PIN, HIGH);
        server.send(200, "application/json", "{\"led\":true}");
    });

    server.on("/api/led/off", HTTP_GET, []() {
        digitalWrite(LED_PIN, LOW);
        server.send(200, "application/json", "{\"led\":false}");
    });

    server.begin();

    Serial.println("Servidor Web iniciado!");
}

void loop() {
    server.handleClient();
}