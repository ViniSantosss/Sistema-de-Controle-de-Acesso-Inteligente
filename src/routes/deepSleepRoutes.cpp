#include "routes/deepSleepRoutes.h"

#include <Arduino.h>
#include <esp_sleep.h>

#include "hardware/doorLock.h"
#include "routes/authRoutes.h"

static constexpr uint32_t TEMPO_RESPOSTA_DEEP_SLEEP_MS = 100;
static constexpr uint32_t DURACAO_DEEP_SLEEP_SEGUNDOS = 60;
static bool deepSleepPendente = false;
static uint32_t deepSleepSolicitadoEm = 0;

void configurarRotasDeepSleep(WebServer& server) {

    server.on("/api/deep-sleep", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }
        if (esp_sleep_enable_timer_wakeup(DURACAO_DEEP_SLEEP_SEGUNDOS * 1000000ULL) != ESP_OK) {
            server.send(500, "application/json",
                        "{\"erro\":\"Nao foi possivel configurar o despertar do ESP32.\"}");
            return;
        }
        Serial.println("Ativando Deep Sleep...");

        server.send(
            200,
            "application/json",
            "{\"mensagem\":\"ESP32 entrando em Deep Sleep\",\"segundos\":" +
                String(DURACAO_DEEP_SLEEP_SEGUNDOS) + "}"
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
    desenergizarTranca();
    Serial.println("Deep Sleep Ativado.");
    esp_deep_sleep_start();
}
