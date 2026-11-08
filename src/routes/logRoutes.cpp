#include "routes/logRoutes.h"

#include <LittleFS.h>

#include "routes/authRoutes.h"
#include "system/systemLogger.h"

namespace {

void enviarArquivoLogs(WebServer& server, bool exportar) {
    if (!administradorAutenticado(server)) {
        server.send(
            401,
            "application/json",
            "{\"erro\":\"Sessao administrativa necessaria.\"}"
        );
        return;
    }

    if (!adquirirAcessoLogs()) {
        server.send(
            503,
            "application/json",
            "{\"erro\":\"Armazenamento de logs indisponivel.\"}"
        );
        return;
    }

    File arquivo = LittleFS.open("/logs.txt", "r");
    if (!arquivo) {
        liberarAcessoLogs();
        server.send(
            500,
            "application/json",
            "{\"erro\":\"Nao foi possivel abrir o historico de logs.\"}"
        );
        return;
    }

    server.sendHeader("Cache-Control", "no-store");
    if (exportar) {
        server.sendHeader(
            "Content-Disposition",
            "attachment; filename=logs.txt"
        );
    }
    server.streamFile(arquivo, "text/plain; charset=utf-8");
    arquivo.close();
    liberarAcessoLogs();
}

}

void configurarRotasLogs(WebServer& server) {
    server.on("/api/logs", HTTP_GET, [&server]() {
        enviarArquivoLogs(server, false);
    });

    server.on("/api/logs/exportar", HTTP_GET, [&server]() {
        enviarArquivoLogs(server, true);
    });
}
