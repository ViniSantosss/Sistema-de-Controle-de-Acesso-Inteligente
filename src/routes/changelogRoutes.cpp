#include "routes/changelogRoutes.h"
#include "routes/authRoutes.h"
#include <ArduinoJson.h>
#include <Preferences.h>

static const char* padrao = R"json([{"data":"2026-11-24","titulo":"Changelog e Deep Sleep","descricao":"Adicionada seção de changelog à página Sobre. Deep Sleep com intervalo configurável e despertar pelo Telegram."},{"data":"","titulo":"Alterações anteriores","descricao":"Implementação de logs do firmware e integração ao painel. Adição de métricas de desempenho das funções. Adição de telemetria ao histórico da porta."}])json";

void configurarRotasChangelog(WebServer& server) {
    server.on("/api/changelog", HTTP_GET, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        Preferences p;
        String dados = padrao;
        if (p.begin("changelog", true)) {
            dados = p.getString("registros", padrao);
            p.end();
        }
        server.send(200, "application/json", dados);
    });
    server.on("/api/changelog", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        JsonDocument doc;
        String corpo = server.arg("plain");
        if (corpo.length() > 16000 || deserializeJson(doc, corpo) ||
            !doc.is<JsonArray>() || doc.size() > 30) {
            server.send(400, "application/json", "{\"erro\":\"Lista invalida ou limite de 30 registros excedido.\"}");
            return;
        }
        for (JsonVariant item : doc.as<JsonArray>()) {
            if (!item.is<JsonObject>() || !item["data"].is<const char*>() ||
                !item["titulo"].is<const char*>() || !item["descricao"].is<const char*>()) {
                server.send(400, "application/json", "{\"erro\":\"Registro invalido.\"}");
                return;
            }
            String data = item["data"].as<String>();
            String titulo = item["titulo"].as<String>();
            String descricao = item["descricao"].as<String>();
            titulo.trim(); descricao.trim();
            if ((data.length() != 0 && (data.length() != 10 || data[4] != '-' || data[7] != '-')) ||
                !titulo.length() || titulo.length() > 120 || !descricao.length() || descricao.length() > 800) {
                server.send(400, "application/json", "{\"erro\":\"Campos invalidos ou muito longos.\"}");
                return;
            }
        }
        String dados;
        serializeJson(doc, dados);
        Preferences p;
        bool salvo = false;
        if (p.begin("changelog", false)) {
            salvo = p.putString("registros", dados) == dados.length();
            p.end();
        }
        if (!salvo) {
            server.send(500, "application/json", "{\"erro\":\"Falha ao salvar changelog na flash.\"}");
            return;
        }
        server.send(200, "application/json", "{\"mensagem\":\"Changelog salvo.\"}");
    });
}
