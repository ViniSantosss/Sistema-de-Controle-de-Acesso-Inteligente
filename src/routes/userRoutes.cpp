#include "routes/userRoutes.h"
#include "telegram/telegramBot.h"

struct Usuario {
    int id;
    String nome;
    String tipoAcesso;
    bool ativo;
    String credencialNFC;
};

int proximoId = 1;

Usuario usuarios[20];
int quantidadeUsuarios = 0;

void configurarRotasUsuarios(WebServer& server) {

    // ROTA POST /api/usuarios --------------------------------------

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
        usuarios[quantidadeUsuarios].ativo = true;
        usuarios[quantidadeUsuarios].credencialNFC = "";
        quantidadeUsuarios++;
        proximoId++;

        notificarUsuarioCadastrado(nome);

        server.send(
            201,
            "application/json",
            "{\"mensagem\":\"Usuario cadastrado com sucesso\"}"
        );
    });

    //ROTA GET /api/usuarios --------------------------------------

    server.on("/api/usuarios", HTTP_GET, [&server]() {

        String json = "[";

        for (int i = 0; i < quantidadeUsuarios; i++) {

            if (i > 0) {
                json += ",";
            }

            json += "{";
            json += "\"id\":" + String(usuarios[i].id) + ",";
            json += "\"nome\":\"" + usuarios[i].nome + "\",";
            json += "\"tipoAcesso\":\"" + usuarios[i].tipoAcesso + "\",";
            json += "\"ativo\":" + String(usuarios[i].ativo ? "true" : "false") + ",";
            json += "\"credencialNfc\":\"" + usuarios[i].credencialNFC + "\"";
            json += "}";
        }

        json += "]";

        server.send(
            200,
            "application/json",
            json
        );
    });

    server.on("/api/usuarios/desativar", HTTP_POST, [&server]() {

        if (!server.hasArg("id")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"ID do usuario e obrigatorio\"}"
            );
            return;
        }

        int id = server.arg("id").toInt();

        for (int i = 0; i < quantidadeUsuarios; i++) {

            if (usuarios[i].id == id) {

                usuarios[i].ativo = false;

                server.send(
                    200,
                    "application/json",
                    "{\"mensagem\":\"Usuario desativado com sucesso\"}"
                );

            return;
            }
        }

            server.send(
                404,
                "application/json",
                "{\"erro\":\"Usuario nao encontrado\"}"
            );
    });
}