#include "routes/performanceRoutes.h"

#include <LittleFS.h>

#include <Arduino.h>
#include <esp_freertos_hooks.h>
#include <esp_timer.h>

static volatile uint32_t totalTicksCore0 = 0;
static volatile uint32_t totalTicksCore1 = 0;
static volatile uint32_t idleTicksCore0 = 0;
static volatile uint32_t idleTicksCore1 = 0;

static uint32_t ultimoTotalCore0 = 0;
static uint32_t ultimoTotalCore1 = 0;
static uint32_t ultimoIdleCore0 = 0;
static uint32_t ultimoIdleCore1 = 0;
static bool monitorPerformanceAtivo = false;

// Tick hooks may run while the flash cache is disabled.
static void IRAM_ATTR tickHookCore0() {
    totalTicksCore0++;
}

static void IRAM_ATTR tickHookCore1() {
    totalTicksCore1++;
}

static bool idleHookCore0() {
    idleTicksCore0++;
    return true;
}

static bool idleHookCore1() {
    idleTicksCore1++;
    return true;
}

void inicializarMonitorPerformance() {
    const esp_err_t tickCore0 = esp_register_freertos_tick_hook_for_cpu(
        tickHookCore0,
        0
    );
    const esp_err_t tickCore1 = esp_register_freertos_tick_hook_for_cpu(
        tickHookCore1,
        1
    );
    const esp_err_t idleCore0 = esp_register_freertos_idle_hook_for_cpu(
        idleHookCore0,
        0
    );
    const esp_err_t idleCore1 = esp_register_freertos_idle_hook_for_cpu(
        idleHookCore1,
        1
    );

    monitorPerformanceAtivo =
        tickCore0 == ESP_OK &&
        tickCore1 == ESP_OK &&
        idleCore0 == ESP_OK &&
        idleCore1 == ESP_OK;

    if (!monitorPerformanceAtivo) {
        Serial.println("Erro ao inicializar o monitor de CPU.");
        return;
    }

    ultimoTotalCore0 = totalTicksCore0;
    ultimoTotalCore1 = totalTicksCore1;
    ultimoIdleCore0 = idleTicksCore0;
    ultimoIdleCore1 = idleTicksCore1;
}

void configurarRotasPerformance(WebServer& server) {
    server.on("/api/performance", HTTP_GET, [&server]() {
        if (!monitorPerformanceAtivo) {
            server.send(
                503,
                "application/json",
                "{\"erro\":\"Monitor de CPU indisponivel\"}"
            );
            return;
        }

        int64_t inicioResposta = esp_timer_get_time();

        uint32_t atualTotalCore0 = totalTicksCore0;
        uint32_t atualTotalCore1 = totalTicksCore1;
        uint32_t atualIdleCore0 = idleTicksCore0;
        uint32_t atualIdleCore1 = idleTicksCore1;

        uint32_t deltaTotalCore0 =
            atualTotalCore0 - ultimoTotalCore0;
        uint32_t deltaTotalCore1 =
            atualTotalCore1 - ultimoTotalCore1;
        uint32_t deltaIdleCore0 =
            atualIdleCore0 - ultimoIdleCore0;
        uint32_t deltaIdleCore1 =
            atualIdleCore1 - ultimoIdleCore1;

        uint32_t idleContadoCore0 =
            deltaIdleCore0 < deltaTotalCore0
                ? deltaIdleCore0
                : deltaTotalCore0;
        uint32_t idleContadoCore1 =
            deltaIdleCore1 < deltaTotalCore1
                ? deltaIdleCore1
                : deltaTotalCore1;

        double usoCore0 = 0;
        double usoCore1 = 0;

        if (deltaTotalCore0 > 0) {
            usoCore0 =
                ((deltaTotalCore0 - idleContadoCore0) * 100.0) /
                deltaTotalCore0;
        }

        if (deltaTotalCore1 > 0) {
            usoCore1 =
                ((deltaTotalCore1 - idleContadoCore1) * 100.0) /
                deltaTotalCore1;
        }

        size_t flashUsada = LittleFS.usedBytes();
        size_t flashTotal = LittleFS.totalBytes();

        size_t ramLivre = ESP.getFreeHeap();
        size_t ramTotal = ESP.getHeapSize();
        size_t ramUsada = ramTotal - ramLivre;

        ultimoTotalCore0 = atualTotalCore0;
        ultimoTotalCore1 = atualTotalCore1;
        ultimoIdleCore0 = atualIdleCore0;
        ultimoIdleCore1 = atualIdleCore1;

        int64_t fimResposta = esp_timer_get_time();
        double tempoResposta =
            (fimResposta - inicioResposta) / 1000.0;

        String json = "{";
        json += "\"core0\":" + String(usoCore0, 1) + ",";
        json += "\"core1\":" + String(usoCore1, 1) + ",";
        json += "\"flashUsada\":" + String(flashUsada) + ",";
        json += "\"flashTotal\":" + String(flashTotal) + ",";
        json += "\"ramLivre\":" + String(ramLivre) + ",";
        json += "\"ramTotal\":" + String(ramTotal) + ",";
        json += "\"ramUsada\":" + String(ramUsada) + ",";
        json += "\"tempoResposta\":" + String(tempoResposta, 2);
        json += "}";

        server.send(
            200,
            "application/json",
            json
        );
    });
}
