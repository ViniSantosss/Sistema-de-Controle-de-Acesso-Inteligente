#include "system/systemTelemetry.h"

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

namespace {

constexpr size_t QUANTIDADE_METRICAS =
    static_cast<size_t>(FunctionMetric::COUNT);

FunctionMetricSnapshot metricas[QUANTIDADE_METRICAS] = {};
portMUX_TYPE mutexMetricas = portMUX_INITIALIZER_UNLOCKED;

}

ScopedFunctionTimer::ScopedFunctionTimer(FunctionMetric metric)
    : metric(metric), startedAt(esp_timer_get_time()) {}

ScopedFunctionTimer::~ScopedFunctionTimer() {
    const int64_t duracao = esp_timer_get_time() - startedAt;
    const uint32_t duracaoMicrosegundos = static_cast<uint32_t>(
        duracao > UINT32_MAX ? UINT32_MAX : duracao
    );
    const size_t indice = static_cast<size_t>(metric);
    if (indice >= QUANTIDADE_METRICAS) {
        return;
    }

    portENTER_CRITICAL(&mutexMetricas);
    FunctionMetricSnapshot& metrica = metricas[indice];
    metrica.calls++;
    metrica.totalMicroseconds += duracaoMicrosegundos;
    if (metrica.calls == 1 || duracaoMicrosegundos < metrica.minimumMicroseconds) {
        metrica.minimumMicroseconds = duracaoMicrosegundos;
    }
    if (duracaoMicrosegundos > metrica.maximumMicroseconds) {
        metrica.maximumMicroseconds = duracaoMicrosegundos;
    }
    portEXIT_CRITICAL(&mutexMetricas);
}

const char* nomeMetricaFuncao(FunctionMetric metric) {
    switch (metric) {
        case FunctionMetric::SENSOR_READ:
            return "Leitura NFC";
        case FunctionMetric::NFC_AUTHENTICATION:
            return "Autenticacao NFC";
        case FunctionMetric::NFC_LINKING:
            return "Vinculacao NFC";
        case FunctionMetric::DOOR_HISTORY:
            return "Processamento do historico da porta";
        case FunctionMetric::WIFI_UPDATE:
            return "Atualizacao da conexao Wi-Fi";
        case FunctionMetric::COUNT:
            break;
    }

    return "Desconhecida";
}

FunctionMetricSnapshot obterMetricaFuncao(FunctionMetric metric) {
    const size_t indice = static_cast<size_t>(metric);
    if (indice >= QUANTIDADE_METRICAS) {
        return {};
    }

    portENTER_CRITICAL(&mutexMetricas);
    const FunctionMetricSnapshot snapshot = metricas[indice];
    portEXIT_CRITICAL(&mutexMetricas);
    return snapshot;
}
