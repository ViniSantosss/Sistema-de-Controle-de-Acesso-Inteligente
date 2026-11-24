#pragma once
#include <Arduino.h>
#include <WebServer.h>

struct SensorConfig {
    uint32_t portaIntervalo;
    uint32_t portaConfirmacao;
    uint32_t nfcIntervalo;
    uint32_t nfcTimeout;
};
SensorConfig obterSensorConfig();
void inicializarSensorConfig();
void configurarRotasSensores(WebServer& server);
