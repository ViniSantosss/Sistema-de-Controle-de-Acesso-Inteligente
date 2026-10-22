#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "routes/statusRoutes.h"
#include "routes/deepSleepRoutes.h"
#include "routes/authRoutes.h"
#include "routes/pageRoutes.h"
#include "routes/userRoutes.h"
#include "telegram/telegramBot.h"
#include "routes/telegramRoutes.h"
#include "storage/userStorage.h"
#include "routes/performanceRoutes.h"
#include "routes/otaRoutes.h"
#include "nfc/nfcManager.h"
#include "sensors/doorHistory.h"

#include "tasks/core0Tasks.h"
#include "tasks/core1Tasks.h"
#include "secrets.h"
#include "time/systemClock.h"

const int LED_PIN = 2;
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WebServer server(80);

static constexpr uint32_t INTERVALO_RECONEXAO_WIFI_MS = 10000;
static uint32_t ultimaTentativaWiFi = 0;
static bool wifiConectado = false;

static void atualizarConexaoWiFi() {
    const uint32_t agora = millis();
    const bool conectado = WiFi.status() == WL_CONNECTED;

    if (conectado && !wifiConectado) {
        Serial.println();
        Serial.println("Wi-Fi conectado!");
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP());
    } else if (!conectado && wifiConectado) {
        Serial.println("Wi-Fi desconectado; tentando reconectar.");
    }

    wifiConectado = conectado;

    if (!conectado &&
        static_cast<uint32_t>(agora - ultimaTentativaWiFi) >=
            INTERVALO_RECONEXAO_WIFI_MS) {
        ultimaTentativaWiFi = agora;
        Serial.println("Tentando reconectar ao Wi-Fi...");
        WiFi.reconnect();
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("================================");
    Serial.println("Sistema de Controle de Acesso");
    Serial.println("ESP32 iniciado!");
    Serial.println("================================");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    ultimaTentativaWiFi = millis();
    iniciarRelogioSistema();

    inicializarArmazenamentoUsuarios();
    inicializarHistoricoPorta();
    inicializarNFC();

    configurarRotasStatus(server);
    configurarRotasAuth(server);
    configurarRotasPaginas(server);
    configurarRotasUsuarios(server);
    configurarRotasTelegram(server);
    configurarRotasDeepSleep(server);
    inicializarMonitorPerformance();
    configurarRotasPerformance(server);
    configurarRotasOta(server);
    iniciarTelegram();

    server.begin();

    iniciarCore0Task();
    iniciarCore1Task();

    Serial.println("Servidor Web iniciado!");
}

void loop() {
    processarHistoricoPortaPendente();
    server.handleClient();
    atualizarConexaoWiFi();
    processarDeepSleepPendente();
    processarReinicioOtaPendente();
}