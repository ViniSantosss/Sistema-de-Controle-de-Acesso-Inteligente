#include "sensors/sensorConfig.h"
#include "routes/authRoutes.h"
#include "system/systemLogger.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>

static SensorConfig config = {10, 5, 100, 200};
static portMUX_TYPE mutexConfig = portMUX_INITIALIZER_UNLOCKED;

SensorConfig obterSensorConfig() {
    portENTER_CRITICAL(&mutexConfig);
    SensorConfig copia = config;
    portEXIT_CRITICAL(&mutexConfig);
    return copia;
}

static void aplicar(const SensorConfig& nova) {
    portENTER_CRITICAL(&mutexConfig);
    config = nova;
    portEXIT_CRITICAL(&mutexConfig);
}

static bool valida(const SensorConfig& c) {
    return c.portaIntervalo >= 10 && c.portaIntervalo <= 1000 &&
           c.portaConfirmacao >= 1 && c.portaConfirmacao <= 20 &&
           c.nfcIntervalo >= 50 && c.nfcIntervalo <= 2000 &&
           c.nfcTimeout >= 100 && c.nfcTimeout <= 1000;
}

void inicializarSensorConfig() {
    Preferences p;
    if (!p.begin("sensores", true)) return;
    SensorConfig salva;
    if (p.getBytesLength("config") == sizeof(salva) &&
        p.getBytes("config", &salva, sizeof(salva)) == sizeof(salva) &&
        valida(salva)) aplicar(salva);
    p.end();
}

void configurarRotasSensores(WebServer& server) {
    server.on("/api/sensores/config", HTTP_GET, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        const SensorConfig c = obterSensorConfig();
        JsonDocument doc;
        doc["portaIntervalo"] = c.portaIntervalo;
        doc["portaConfirmacao"] = c.portaConfirmacao;
        doc["nfcIntervalo"] = c.nfcIntervalo;
        doc["nfcTimeout"] = c.nfcTimeout;
        String resposta;
        serializeJson(doc, resposta);
        server.send(200, "application/json", resposta);
    });
    server.on("/api/sensores/config", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        JsonDocument doc;
        if (deserializeJson(doc, server.arg("plain")) ||
            !doc["portaIntervalo"].is<uint32_t>() ||
            !doc["portaConfirmacao"].is<uint32_t>() ||
            !doc["nfcIntervalo"].is<uint32_t>() ||
            !doc["nfcTimeout"].is<uint32_t>()) {
            server.send(400, "application/json", "{\"erro\":\"Parametros invalidos.\"}");
            return;
        }
        SensorConfig nova = {doc["portaIntervalo"].as<uint32_t>(),
            doc["portaConfirmacao"].as<uint32_t>(),
            doc["nfcIntervalo"].as<uint32_t>(),
            doc["nfcTimeout"].as<uint32_t>()};
        if (!valida(nova)) {
            server.send(400, "application/json", "{\"erro\":\"Valores fora dos limites permitidos.\"}");
            return;
        }
        Preferences p;
        bool salvo = false;
        if (p.begin("sensores", false)) {
            salvo = p.putBytes("config", &nova, sizeof(nova)) == sizeof(nova);
            p.end();
        }
        if (!salvo) {
            server.send(500, "application/json", "{\"erro\":\"Falha ao salvar configuracao.\"}");
            return;
        }
        aplicar(nova);
        registrarLog(LogLevel::INFO, "Configuracao dos sensores atualizada.");
        server.send(200, "application/json", "{\"mensagem\":\"Configuracao salva e aplicada.\"}");
    });
}
