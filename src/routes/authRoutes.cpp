#include "routes/authRoutes.h"

const char* USUARIO = "admin";
const char* SENHA = "1234";

void configurarRotasAuth(WebServer& server) {

    server.on("/api/login", HTTP_GET, [&server]() {

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

        if (usuario == USUARIO && senha == SENHA) {
            server.send(
                200,
                "application/json",
                "{\"autenticado\":true}"
            );
        } else {
            server.send(
                401,
                "application/json",
                "{\"autenticado\":false}"
            );
        }
    });
}