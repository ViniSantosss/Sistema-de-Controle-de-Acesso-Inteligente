#include "routes/deepSleepRoutes.h"

#include <Arduino.h>
#include <esp_sleep.h>

static constexpr uint32_t TEMPO_RESPOSTA_DEEP_SLEEP_MS = 100;
static bool deepSleepPendente = false;
static uint32_t deepSleepSolicitadoEm = 0;

void configurarRotasDeepSleep(WebServer& server) {

    server.on("/api/deep-sleep", HTTP_POST, [&server]() {
        Serial.println("Ativando Deep Sleep...");

        server.send(
            200,
            "application/json",
            "{\"mensagem\":\"ESP32 entrando em Deep Sleep\"}"
        );

        deepSleepSolicitadoEm = millis();
        deepSleepPendente = true;
    });
}

void processarDeepSleepPendente() {
    if (!deepSleepPendente ||
        static_cast<uint32_t>(millis() - deepSleepSolicitadoEm) <
            TEMPO_RESPOSTA_DEEP_SLEEP_MS) {
        return;
    }

    deepSleepPendente = false;
    Serial.println("Deep Sleep Ativado.");
    esp_deep_sleep_start();
}