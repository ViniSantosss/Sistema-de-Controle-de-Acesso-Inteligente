#include "routes/deepSleepRoutes.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "hardware/doorLock.h"
#include "routes/authRoutes.h"
#include "telegram/telegramBot.h"

static constexpr uint32_t TEMPO_RESPOSTA_DEEP_SLEEP_MS = 100;
static RTC_DATA_ATTR bool cicloAtivo = false;
static RTC_DATA_ATTR uint32_t intervaloRTC = 30;
static RTC_DATA_ATTR long ultimoUpdate = 0;
static bool deepSleepPendente = false;
static uint32_t deepSleepSolicitadoEm = 0;
static uint32_t intervalo = 30;

static bool intervaloValido(uint32_t valor) {
    return valor == 15 || valor == 30 || valor == 60 ||
           valor == 300 || valor == 600;
}

static void carregarIntervalo() {
    Preferences preferencias;
    if (preferencias.begin("deep-sleep", true)) {
        uint32_t valor = preferencias.getUInt("intervalo", 30);
        if (intervaloValido(valor)) intervalo = valor;
        preferencias.end();
    }
}

static bool prepararDespertar(uint32_t segundos) {
    return esp_sleep_enable_timer_wakeup(segundos * 1000000ULL) == ESP_OK;
}

void verificarDespertarDeepSleep() {
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER || !cicloAtivo) {
        cicloAtivo = false;
        ultimoUpdate = 0;
        return;
    }

    const uint32_t inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < 10000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED && consultarDespertarTelegram(ultimoUpdate)) {
        cicloAtivo = false;
        return;
    }

    if (prepararDespertar(intervaloRTC)) {
        desenergizarTranca();
        esp_deep_sleep_start();
    }
    cicloAtivo = false;
}

void configurarRotasDeepSleep(WebServer& server) {
    carregarIntervalo();

    server.on("/api/deep-sleep", HTTP_GET, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        server.send(200, "application/json", "{\"segundos\":" + String(intervalo) + "}");
    });

    server.on("/api/deep-sleep", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        if (deepSleepPendente) {
            server.send(409, "application/json", "{\"erro\":\"Suspensao ja solicitada.\"}");
            return;
        }
        JsonDocument dados;
        if (deserializeJson(dados, server.arg("plain")) ||
            !dados["segundos"].is<uint32_t>() ||
            !intervaloValido(dados["segundos"].as<uint32_t>())) {
            server.send(400, "application/json", "{\"erro\":\"Intervalo invalido.\"}");
            return;
        }
        const uint32_t valor = dados["segundos"].as<uint32_t>();
        Preferences preferencias;
        if (!preferencias.begin("deep-sleep", false)) {
            server.send(500, "application/json", "{\"erro\":\"Falha ao abrir configuracoes.\"}");
            return;
        }
        const bool salvo = preferencias.putUInt("intervalo", valor) == sizeof(uint32_t);
        preferencias.end();
        if (!salvo || !prepararDespertar(valor)) {
            server.send(500, "application/json", "{\"erro\":\"Falha ao configurar suspensao.\"}");
            return;
        }
        intervalo = valor;
        intervaloRTC = valor;
        ultimoUpdate = 0;
        cicloAtivo = true;
        server.send(200, "application/json", "{\"segundos\":" + String(intervalo) + "}");
        deepSleepSolicitadoEm = millis();
        deepSleepPendente = true;
    });
}

void processarDeepSleepPendente() {
    if (!deepSleepPendente || millis() - deepSleepSolicitadoEm < TEMPO_RESPOSTA_DEEP_SLEEP_MS) return;
    deepSleepPendente = false;
    desenergizarTranca();
    esp_deep_sleep_start();
}
