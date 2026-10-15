#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "routes/statusRoutes.h"
#include "routes/ledRoutes.h"
#include "routes/authRoutes.h"

const int LED_PIN = 2;
const char* ssid = "busquem comer cimento";
const char* password = "123456788sixseven";

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

    server.on("/icons.svg", HTTP_GET, []() {
        File file = LittleFS.open("/icons.svg", "r");

        if (!file) {
            server.send(404, "text/plain", "Icones nao encontrados");
            return;
        }

        server.streamFile(file, "image/svg+xml");
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

    server.on("/panel.html", HTTP_GET, []() {
        File file = LittleFS.open("/panel.html", "r");

        if (!file) {
            server.send(404, "text/plain", "panel.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/users.html", HTTP_GET, []() {
        File file = LittleFS.open("/users.html", "r");

        if (!file) {
            server.send(404, "text/plain", "users.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/config.html", HTTP_GET, []() {
        File file = LittleFS.open("/config.html", "r");

        if (!file) {
            server.send(404, "text/plain", "configuracoes.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/performance.html", HTTP_GET, []() {
        File file = LittleFS.open("/performance.html", "r");

        if (!file) {
            server.send(404, "text/plain", "performance.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    configurarRotasStatus(server);
    configurarRotasLed(server, LED_PIN);
    configurarRotasAuth(server);

    server.begin();

    Serial.println("Servidor Web iniciado!");
}

void loop() {
    server.handleClient();
}