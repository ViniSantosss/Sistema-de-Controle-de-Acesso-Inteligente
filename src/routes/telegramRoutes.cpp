#include <Arduino.h>
#include <WebServer.h>

#include "routes/telegramRoutes.h"
#include "telegram/telegramBot.h"

void configurarRotasTelegram(WebServer& server) {

    server.on("/api/telegram/teste", HTTP_GET, [&server]() {

        bool enviado = enviarMensagemTelegram(
            "Teste do Sistema de Controle de Acesso."
        );

        if (!enviado) {
            server.send(
                500,
                "application/json",
                "{\"erro\":\"Nao foi possivel enviar a mensagem\"}"
            );
            return;
        }

        server.send(
            200,
            "application/json",
            "{\"mensagem\":\"Mensagem enviada com sucesso\"}"
        );
    });
}