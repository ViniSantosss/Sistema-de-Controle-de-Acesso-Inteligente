#ifndef SYSTEM_TELEMETRY_H
#define SYSTEM_TELEMETRY_H

#include <Arduino.h>

enum class FunctionMetric : uint8_t {
    SENSOR_READ,
    NFC_AUTHENTICATION,
    NFC_LINKING,
    DOOR_HISTORY,
    WIFI_UPDATE,
    COUNT
};

struct FunctionMetricSnapshot {
    uint64_t calls;
    uint64_t totalMicroseconds;
    uint32_t minimumMicroseconds;
    uint32_t maximumMicroseconds;
};

class ScopedFunctionTimer {
public:
    explicit ScopedFunctionTimer(FunctionMetric metric);
    ~ScopedFunctionTimer();

    ScopedFunctionTimer(const ScopedFunctionTimer&) = delete;
    ScopedFunctionTimer& operator=(const ScopedFunctionTimer&) = delete;

private:
    FunctionMetric metric;
    int64_t startedAt;
};

const char* nomeMetricaFuncao(FunctionMetric metric);
FunctionMetricSnapshot obterMetricaFuncao(FunctionMetric metric);

#endif
