#include "routes/authRoutes.h"
#include <Arduino.h>
#include <esp_system.h>
#include "secrets.h"

static const char NOME_COOKIE_SESSAO[] = "esp32_admin_session";
static String tokenSessaoAdministrador;

static String criarTokenSessao() {
    char token[33];
    for (size_t i = 0; i < 4; i++) {
        snprintf(
            token + i * 8,
            sizeof(token) - i * 8,
            "%08lx",
            static_cast<unsigned long>(esp_random())
        );
    }
    return String(token);
}

bool administradorAutenticado(WebServer& server) {
    if (tokenSessaoAdministrador.isEmpty()) {
        return false;
    }

    const String cookies = server.header("Cookie");
    const String prefixo = String(NOME_COOKIE_SESSAO) + "=";
    int inicio = 0;

    while (inicio < cookies.length()) {
        int fim = cookies.indexOf(';', inicio);
        if (fim < 0) {
            fim = cookies.length();
        }

        String cookie = cookies.substring(inicio, fim);
        cookie.trim();
        if (cookie.startsWith(prefixo)) {
            const String tokenRecebido = cookie.substring(prefixo.length());
            if (tokenRecebido.length() != tokenSessaoAdministrador.length()) {
                return false;
            }

            uint8_t diferenca = 0;
            for (size_t i = 0; i < tokenRecebido.length(); i++) {
                diferenca |= static_cast<uint8_t>(
                    tokenRecebido[i] ^ tokenSessaoAdministrador[i]
                );
            }
            return diferenca == 0;
        }

        inicio = fim + 1;
    }

    return false;
}

void configurarRotasAuth(WebServer& server) {
    const char* cabecalhos[] = {"Cookie"};
    server.collectHeaders(cabecalhos, 1);

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
            tokenSessaoAdministrador = criarTokenSessao();
            server.sendHeader(
                "Set-Cookie",
                String(NOME_COOKIE_SESSAO) + "=" + tokenSessaoAdministrador +
                    "; Path=/; HttpOnly; SameSite=Strict"
            );
            server.sendHeader("Cache-Control", "no-store");
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