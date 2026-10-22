#include "routes/userRoutes.h"
#include "telegram/telegramBot.h"
#include "storage/userStorage.h"
#include "models/user.h"
#include "time/systemClock.h"

#include <ArduinoJson.h>
#include <time.h>

static bool dadosDeAcessoValidos(
    const String& tipoAcesso,
    const String& dataExpiracao
) {
    if (tipoAcesso == "PERMANENTE") {
        return true;
    }

    time_t instante;
    return tipoAcesso == "TEMPORARIO" &&
        !dataExpiracao.isEmpty() &&
        interpretarDataExpiracao(dataExpiracao, instante);
}

static String descricaoTipoAcesso(const String& tipoAcesso) {
    return tipoAcesso == "PERMANENTE" ? "Permanente" : "Temporário";
}

static String respostaJson(const JsonDocument& documento) {
    String resposta;
    serializeJson(documento, resposta);
    return resposta;
}

int proximoId = 1;

Usuario usuarios[MAX_USUARIOS];
int quantidadeUsuarios = 0;
bool aguardandoChaveNFC = false;
int usuarioAguardandoChave = -1;

bool estaAguardandoChaveNFC() {
    return aguardandoChaveNFC;
}

int getUsuarioAguardandoChave() {
    return usuarioAguardandoChave;
}

void finalizarAguardandoChaveNFC() {
    aguardandoChaveNFC = false;
    usuarioAguardandoChave = -1;
}

ResultadoVinculacaoNFC vincularChaveNFC(
    const String& uid,
    String& nomeUsuario
) {

    int idUsuario = getUsuarioAguardandoChave();

    if (idUsuario == -1) {
        return ResultadoVinculacaoNFC::FALHA;
    }

    int indiceUsuario = -1;
    for (int i = 0; i < quantidadeUsuarios; i++) {
        if (usuarios[i].id == idUsuario) {
            indiceUsuario = i;
            nomeUsuario = usuarios[i].nome;
            break;
        }
    }

    if (indiceUsuario == -1) {
        finalizarAguardandoChaveNFC();
        return ResultadoVinculacaoNFC::FALHA;
    }

    for (int i = 0; i < quantidadeUsuarios; i++) {
        if (usuarios[i].id == idUsuario &&
            usuarios[i].credencialNFC == uid) {
            finalizarAguardandoChaveNFC();
            return ResultadoVinculacaoNFC::SUCESSO;
        }

        if (usuarios[i].id != idUsuario &&
            usuarios[i].credencialNFC == uid) {
            finalizarAguardandoChaveNFC();
            return ResultadoVinculacaoNFC::CHAVE_JA_CADASTRADA;
        }
    }

    String credencialAnterior = usuarios[indiceUsuario].credencialNFC;
    usuarios[indiceUsuario].credencialNFC = uid;

    if (!salvarUsuarios()) {
        usuarios[indiceUsuario].credencialNFC = credencialAnterior;
        finalizarAguardandoChaveNFC();
        return ResultadoVinculacaoNFC::FALHA;
    }

    finalizarAguardandoChaveNFC();
    return ResultadoVinculacaoNFC::SUCESSO;
}

ResultadoAutenticacaoNFC autenticarChaveNFC(
    const String& uid,
    String& nomeUsuario,
    String& tipoAcesso,
    String& dataExpiracao
) {

    for (int i = 0; i < quantidadeUsuarios; i++) {

        if (!usuarios[i].credencialNFC.isEmpty() &&
            usuarios[i].credencialNFC == uid) {
            nomeUsuario = usuarios[i].nome;
            tipoAcesso = usuarios[i].tipoAcesso;
            dataExpiracao = usuarios[i].dataExpiracao;

            if (!usuarios[i].ativo) {
                Serial.print("Acesso negado: usuario desativado - ");
                Serial.println(usuarios[i].nome);

                return ResultadoAutenticacaoNFC::USUARIO_DESATIVADO;
            }

            if (tipoAcesso == "TEMPORARIO") {
                time_t instanteExpiracao;
                if (!horarioSistemaConfiavel() ||
                    !interpretarDataExpiracao(
                        dataExpiracao,
                        instanteExpiracao
                    )) {
                    Serial.println(
                        "Acesso negado: validade temporal nao verificavel."
                    );
                    return ResultadoAutenticacaoNFC::VALIDADE_INDISPONIVEL;
                }

                if (time(nullptr) > instanteExpiracao) {
                    Serial.print("Acesso negado: validade expirada - ");
                    Serial.println(nomeUsuario);
                    return ResultadoAutenticacaoNFC::CHAVE_EXPIRADA;
                }
            }

            Serial.print("Acesso autorizado: ");
            Serial.println(usuarios[i].nome);

            return ResultadoAutenticacaoNFC::AUTORIZADO;
        }
    }

    Serial.print("Acesso negado: chave nao cadastrada - ");
    Serial.println(uid);

    return ResultadoAutenticacaoNFC::CHAVE_DESCONHECIDA;
}

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
        String dataExpiracao = server.arg("dataExpiracao");

        if (tipoAcesso != "PERMANENTE" &&
            tipoAcesso != "TEMPORARIO") {

            server.send(
                400,
                "application/json",
                "{\"erro\":\"Tipo de acesso invalido\"}"
            );
            return;
        }

        if (!dadosDeAcessoValidos(tipoAcesso, dataExpiracao)) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"Informe uma data e hora de expiracao validas para o acesso temporario\"}"
            );
            return;
        }

        if (quantidadeUsuarios >= MAX_USUARIOS) {
            server.send(
                409,
                "application/json",
                "{\"erro\":\"Limite de usuarios atingido\"}"
            );
            return;
        }

        usuarios[quantidadeUsuarios].id = proximoId;
        usuarios[quantidadeUsuarios].nome = nome;
        usuarios[quantidadeUsuarios].tipoAcesso = tipoAcesso;
        usuarios[quantidadeUsuarios].ativo = true;
        usuarios[quantidadeUsuarios].credencialNFC = "";
        usuarios[quantidadeUsuarios].dataExpiracao =
            tipoAcesso == "TEMPORARIO" ? dataExpiracao : "";
        quantidadeUsuarios++;
        proximoId++;

        if (!salvarUsuarios()) {
            quantidadeUsuarios--;
            proximoId--;
            server.send(
                500,
                "application/json",
                "{\"erro\":\"Nao foi possivel persistir o usuario\"}"
            );
            return;
        }

        const String tipoDescricao = descricaoTipoAcesso(tipoAcesso);
        notificarUsuarioCadastrado(nome, tipoDescricao);

        JsonDocument resposta;
        resposta["mensagem"] =
            "Usuário " + nome + " criado. Tipo de acesso: " +
            tipoDescricao + ".";
        server.send(
            201,
            "application/json",
            respostaJson(resposta)
        );
    });

    server.on("/api/usuarios", HTTP_PUT, [&server]() {
        if (!server.hasArg("id") ||
            !server.hasArg("nome") ||
            !server.hasArg("tipoAcesso")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"ID, nome e tipo de acesso sao obrigatorios\"}"
            );
            return;
        }

        const int id = server.arg("id").toInt();
        const String nome = server.arg("nome");
        const String tipoAcesso = server.arg("tipoAcesso");
        const String dataExpiracao = server.arg("dataExpiracao");

        if ((tipoAcesso != "PERMANENTE" &&
             tipoAcesso != "TEMPORARIO") ||
            !dadosDeAcessoValidos(tipoAcesso, dataExpiracao)) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"Tipo de acesso ou data e hora de expiracao invalidos\"}"
            );
            return;
        }

        for (int i = 0; i < quantidadeUsuarios; i++) {
            if (usuarios[i].id != id) {
                continue;
            }

            const Usuario usuarioAnterior = usuarios[i];
            usuarios[i].nome = nome;
            usuarios[i].tipoAcesso = tipoAcesso;
            usuarios[i].dataExpiracao =
                tipoAcesso == "TEMPORARIO" ? dataExpiracao : "";

            if (!salvarUsuarios()) {
                usuarios[i] = usuarioAnterior;
                server.send(
                    500,
                    "application/json",
                    "{\"erro\":\"Nao foi possivel persistir as alteracoes do usuario\"}"
                );
                return;
            }

            JsonDocument resposta;
            resposta["mensagem"] = "Usuario atualizado com sucesso.";
            server.send(
                200,
                "application/json",
                respostaJson(resposta)
            );
            return;
        }

        server.send(
            404,
            "application/json",
            "{\"erro\":\"Usuario nao encontrado\"}"
        );
    });

    //ROTA GET /api/usuarios --------------------------------------

    server.on("/api/usuarios", HTTP_GET, [&server]() {

        JsonDocument documento;
        JsonArray lista = documento.to<JsonArray>();
        const bool horarioConfiavel = horarioSistemaConfiavel();
        const time_t agora = time(nullptr);

        for (int i = 0; i < quantidadeUsuarios; i++) {
            JsonObject usuario = lista.add<JsonObject>();
            time_t instanteExpiracao;
            const bool acessoTemporario =
                usuarios[i].tipoAcesso == "TEMPORARIO";
            const bool validadeConfigurada =
                !usuarios[i].dataExpiracao.isEmpty() &&
                interpretarDataExpiracao(
                    usuarios[i].dataExpiracao,
                    instanteExpiracao
                );

            usuario["id"] = usuarios[i].id;
            usuario["nome"] = usuarios[i].nome;
            usuario["tipoAcesso"] = usuarios[i].tipoAcesso;
            usuario["ativo"] = usuarios[i].ativo;
            usuario["credencialNfc"] = usuarios[i].credencialNFC;
            usuario["dataExpiracao"] = usuarios[i].dataExpiracao;
            usuario["expirado"] = acessoTemporario &&
                horarioConfiavel &&
                validadeConfigurada &&
                agora > instanteExpiracao;
            usuario["validadeVerificavel"] = !acessoTemporario ||
                (horarioConfiavel && validadeConfigurada);
            usuario["dataExpiracaoFormatada"] =
                formatarDataExpiracao(usuarios[i].dataExpiracao);
        }

        server.send(
            200,
            "application/json",
            respostaJson(documento)
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

                salvarUsuarios();

                notificarUsuarioDesativado(usuarios[i].nome);

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

    server.on("/api/usuarios", HTTP_DELETE, [&server]() {

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

                String nomeUsuario = usuarios[i].nome;

                // Move os usuarios seguintes uma posicao para tras
                for (int j = i; j < quantidadeUsuarios - 1; j++) {
                    usuarios[j] = usuarios[j + 1];
                }

                quantidadeUsuarios--;

                salvarUsuarios();

                notificarUsuarioExcluido(nomeUsuario);

                server.send(
                    200,
                    "application/json",
                    "{\"mensagem\":\"Usuario excluido com sucesso\"}"
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

    server.on("/api/usuarios/aguardar-chave", HTTP_POST, [&server]() {

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

                if (!usuarios[i].ativo) {
                    server.send(
                        400,
                        "application/json",
                        "{\"erro\":\"Usuario esta desativado\"}"
                    );
                    return;
                }

                aguardandoChaveNFC = true;
                usuarioAguardandoChave = id;

                server.send(
                    200,
                    "application/json",
                    "{\"mensagem\":\"Aproxime a chave NFC do leitor\"}"
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