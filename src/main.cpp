#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "routes/statusRoutes.h"
#include "routes/deepSleepRoutes.h"
#include "routes/ledRoutes.h"
#include "routes/authRoutes.h"
#include "routes/pageRoutes.h"
#include "routes/userRoutes.h"
#include "telegram/telegramBot.h"
#include "routes/telegramRoutes.h"
#include "storage/userStorage.h"
#include "routes/performanceRoutes.h"
#include "secrets.h"

const int LED_PIN = 2;
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

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

    inicializarArmazenamentoUsuarios();

    configurarRotasStatus(server);
    configurarRotasLed(server, LED_PIN);
    configurarRotasAuth(server);
    configurarRotasPaginas(server);
    configurarRotasUsuarios(server);
    configurarRotasTelegram(server);
    configurarRotasDeepSleep(server);
    inicializarMonitorPerformance();
    configurarRotasPerformance(server);
    iniciarTelegram();

    server.begin();

    Serial.println("Servidor Web iniciado!");
}

void loop() {
    server.handleClient();
}