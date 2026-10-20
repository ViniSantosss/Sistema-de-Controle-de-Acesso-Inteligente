#include "routes/authRoutes.h"
#include "secrets.h"

void configurarRotasAuth(WebServer& server) {

    server.on("/api/login", HTTP_POST, [&server]() {

        if (!server.hasArg("usuario") || !server.hasArg("senha")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"Usuario e senha obrigatorios\"}"
            );
            return;
        }

        String usuario = server.arg("usuario");
        String senha = server.arg("senha");

        if (usuario == ESP32_USER && senha == ESP32_PASSWORD) {
            server.send(
                200,
                "application/json",
                "{\"autenticado\":true}"
            );
        } else {
            server.send(
                401,
                "application/json",
                "{\"autenticado\":false,\"erro\":\"Usuário ou senha inválidos\"}"
            );
        }
    });
}