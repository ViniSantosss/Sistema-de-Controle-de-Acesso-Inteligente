#include "routes/userRoutes.h"

struct Usuario {
    int id;
    String nome;
    String tipoAcesso;
};

int proximoId = 1;

Usuario usuarios[20];
int quantidadeUsuarios = 0;

void configurarRotasUsuarios(WebServer& server) {

    server.on("/api/usuarios", HTTP_POST, [&server]() {

        if (!server.hasArg("nome") || !server.hasArg("tipoAcesso")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"Nome e tipo de acesso sao obrigatorios\"}"
            );
            return;
        }

        String nome = server.arg("nome");
        String tipoAcesso = server.arg("tipoAcesso");

        if (tipoAcesso != "PERMANENTE" &&
            tipoAcesso != "TEMPORARIO") {

            server.send(
                400,
                "application/json",
                "{\"erro\":\"Tipo de acesso invalido\"}"
            );
            return;
        }

        usuarios[quantidadeUsuarios].id = proximoId;
        usuarios[quantidadeUsuarios].nome = nome;
        usuarios[quantidadeUsuarios].tipoAcesso = tipoAcesso;

        quantidadeUsuarios++;
        proximoId++;

        server.send(
            201,
            "application/json",
            "{\"mensagem\":\"Usuario cadastrado com sucesso\"}"
        );
    });

    server.on("/api/usuarios", HTTP_GET, [&server]() {

        String json = "[";

        for (int i = 0; i < quantidadeUsuarios; i++) {

            if (i > 0) {
                json += ",";
            }

            json += "{";
            json += "\"id\":" + String(usuarios[i].id) + ",";
            json += "\"nome\":\"" + usuarios[i].nome + "\",";
            json += "\"tipoAcesso\":\"" + usuarios[i].tipoAcesso + "\"";
            json += "}";
        }

        json += "]";

        server.send(
            200,
            "application/json",
            json
        );
    });
}