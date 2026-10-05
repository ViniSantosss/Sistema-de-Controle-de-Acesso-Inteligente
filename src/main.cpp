#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "nome_generico_da_sua_rede";
const char* password = "senha_generica_da_sua_rede";

WebServer server(80);

void handleRoot() {
    server.send(200, "text/html", "<h1>Sistema de Controle de Acesso</h1>");
}

void setup() {
    Serial.begin(115200);

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

    server.on("/", handleRoot);

    server.begin();

    Serial.println("Servidor Web iniciado!");
}

void loop() {
    server.handleClient();
}