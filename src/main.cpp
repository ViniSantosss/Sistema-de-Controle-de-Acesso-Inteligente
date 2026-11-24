#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "routes/statusRoutes.h"
#include "network/wifiManager.h"
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
#include "hardware/doorLock.h"
#include "sensors/doorHistory.h"
#include "sensors/sensorConfig.h"

#include "tasks/core0Tasks.h"
#include "tasks/core1Tasks.h"
#include "secrets.h"
#include "time/systemClock.h"
#include "system/systemLogger.h"
#include "system/systemTelemetry.h"
#include "routes/logRoutes.h"
#include "routes/changelogRoutes.h"

const int LED_PIN = 2;

WebServer server(80);

static constexpr uint32_t INTERVALO_RECONEXAO_WIFI_MS = 10000;
static uint32_t ultimaTentativaWiFi = 0;
static bool wifiConectado = false;

static void atualizarConexaoWiFi() {
    ScopedFunctionTimer timer(FunctionMetric::WIFI_UPDATE);
    const uint32_t agora = millis();
    const bool conectado = WiFi.status() == WL_CONNECTED;

    if (conectado && !wifiConectado) {
        Serial.println();
        Serial.println("Wi-Fi conectado!");
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP());
        registrarLog(LogLevel::INFO, "Wi-Fi conectado; IP " + WiFi.localIP().toString());
    } else if (!conectado && wifiConectado) {
        Serial.println("Wi-Fi desconectado; tentando reconectar.");
        registrarLog(LogLevel::WARNING, "Wi-Fi desconectado; tentando reconectar.");
    }

    wifiConectado = conectado;

    if (!conectado &&
        static_cast<uint32_t>(agora - ultimaTentativaWiFi) >=
            INTERVALO_RECONEXAO_WIFI_MS) {
        ultimaTentativaWiFi = agora;
        Serial.println("Tentando reconectar ao Wi-Fi...");
        // A reconexao automatica e gerenciada pelo Wi-Fi.
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    inicializarTranca();

    Serial.println();
    Serial.println("================================");
    Serial.println("Sistema de Controle de Acesso");
    Serial.println("ESP32 iniciado!");
    Serial.println("================================");

    iniciarWiFi();
    ultimaTentativaWiFi = millis();
    verificarDespertarDeepSleep();
    iniciarRelogioSistema();

    inicializarArmazenamentoUsuarios();
    if (!inicializarLogs()) {
        Serial.println("Sistema de logs persistentes indisponivel.");
    } else {
        registrarLog(LogLevel::INFO, "Sistema iniciado.");
    }

    inicializarHistoricoPorta();
    inicializarSensorConfig();
    inicializarNFC();

    configurarRotasStatus(server);
    configurarRotasAuth(server);
    configurarRotasWiFi(server);
    configurarRotasSensores(server);
    configurarRotasPaginas(server);
    configurarRotasUsuarios(server);
    configurarRotasTelegram(server);
    configurarRotasDeepSleep(server);
    inicializarMonitorPerformance();
    configurarRotasPerformance(server);
    configurarRotasOta(server);
    configurarRotasLogs(server);
    configurarRotasChangelog(server);
    iniciarTelegram();

    server.begin();

    iniciarTasksCore0();
    iniciarTasksCore1();

    Serial.println("Servidor Web iniciado!");
    registrarLog(LogLevel::INFO, "Servidor web iniciado.");
}

void loop() {
    static uint32_t ultimoHistorico = 0;
    static uint32_t ultimoWiFi = 0;

    server.handleClient();
    const uint32_t agora = millis();

    if (agora - ultimoHistorico >= 100) {
        ultimoHistorico = agora;
        processarHistoricoPortaPendente();
    }

    if (agora - ultimoWiFi >= 1000) {
        ultimoWiFi = agora;
        atualizarModoAP();
        atualizarConexaoWiFi();
    }

    processarDeepSleepPendente();
    processarReinicioOtaPendente();
    // Libera o core para outras tasks sem espera ocupada.
    vTaskDelay(1);
}
