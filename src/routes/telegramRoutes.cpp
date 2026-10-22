#include <Arduino.h>
#include <WebServer.h>

#include "routes/telegramRoutes.h"
#include "telegram/telegramBot.h"

void configurarRotasTelegram(WebServer& server) {

    server.on("/api/telegram/teste", HTTP_GET, [&server]() {

        bool enfileirado = enviarMensagemTelegram(
            "Teste do Sistema de Controle de Acesso."
        );

        if (!enfileirado) {
            server.send(
                503,
                "application/json",
                "{\"erro\":\"Nao foi possivel enfileirar a mensagem\"}"
            );
            return;
        }

        server.send(
            202,
            "application/json",
            "{\"mensagem\":\"Mensagem adicionada a fila de envio\"}"
        );
    });
}