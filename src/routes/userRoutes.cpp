#include "routes/userRoutes.h"
#include "routes/authRoutes.h"
#include "hardware/doorLock.h"
#include "system/systemTelemetry.h"
#include "telegram/telegramBot.h"
#include "storage/userStorage.h"
#include "models/user.h"
#include "time/systemClock.h"

#include <ArduinoJson.h>
#include <esp_timer.h>
#include <mutex>
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
// Protege o cadastro e a gravação no LittleFS entre as rotas HTTP e a tarefa NFC.
static std::mutex mutexUsuarios;
static constexpr int64_t PRAZO_VINCULACAO_NFC_US = 30000000;
static int usuarioAguardandoChave = -1;
static int64_t vinculacaoExpiraEm = 0;

static bool estaAguardandoChaveNFC() {
    return usuarioAguardandoChave != -1 && esp_timer_get_time() < vinculacaoExpiraEm;
}

static ResultadoLeituraNFC vincularChaveNFC(
    const String& uid,
    String& nomeUsuario
) {
    ScopedFunctionTimer timer(FunctionMetric::NFC_LINKING);

    const int idUsuario = usuarioAguardandoChave;
    usuarioAguardandoChave = -1;

    int indiceUsuario = -1;
    for (int i = 0; i < quantidadeUsuarios; i++) {
        if (usuarios[i].id == idUsuario && usuarios[i].ativo) {
            indiceUsuario = i;
            nomeUsuario = usuarios[i].nome;
            break;
        }
    }

    if (indiceUsuario == -1) {
        return ResultadoLeituraNFC::VINCULACAO_FALHOU;
    }

    for (int i = 0; i < quantidadeUsuarios; i++) {
        if (usuarios[i].id == idUsuario &&
            usuarios[i].credencialNFC == uid) {
            return ResultadoLeituraNFC::CHAVE_VINCULADA;
        }

        if (usuarios[i].id != idUsuario &&
            usuarios[i].credencialNFC == uid) {
            return ResultadoLeituraNFC::CHAVE_JA_CADASTRADA;
        }
    }

    String credencialAnterior = usuarios[indiceUsuario].credencialNFC;
    usuarios[indiceUsuario].credencialNFC = uid;

    if (!salvarUsuarios()) {
        usuarios[indiceUsuario].credencialNFC = credencialAnterior;
        return ResultadoLeituraNFC::VINCULACAO_FALHOU;
    }

    return ResultadoLeituraNFC::CHAVE_VINCULADA;
}

static ResultadoLeituraNFC autenticarChaveNFC(
    const String& uid,
    String& nomeUsuario,
    String& tipoAcesso,
    String& dataExpiracao
) {
    ScopedFunctionTimer timer(FunctionMetric::NFC_AUTHENTICATION);

    for (int i = 0; i < quantidadeUsuarios; i++) {

        if (!usuarios[i].credencialNFC.isEmpty() &&
            usuarios[i].credencialNFC == uid) {
            nomeUsuario = usuarios[i].nome;
            tipoAcesso = usuarios[i].tipoAcesso;
            dataExpiracao = usuarios[i].dataExpiracao;

            if (!usuarios[i].ativo) {
                return ResultadoLeituraNFC::USUARIO_DESATIVADO;
            }

            if (tipoAcesso == "TEMPORARIO") {
                time_t instanteExpiracao;
                if (!horarioSistemaConfiavel() ||
                    !interpretarDataExpiracao(
                        dataExpiracao,
                        instanteExpiracao
                    )) {
                    return ResultadoLeituraNFC::VALIDADE_INDISPONIVEL;
                }

                if (time(nullptr) > instanteExpiracao) {
                    return ResultadoLeituraNFC::CHAVE_EXPIRADA;
                }
            }

            return ResultadoLeituraNFC::AUTORIZADO;
        }
    }

    return ResultadoLeituraNFC::CHAVE_DESCONHECIDA;
}

ResultadoLeituraNFC processarLeituraNFC(
    const String& uid,
    String& nomeUsuario,
    String& tipoAcesso,
    String& dataExpiracao
) {
    const std::lock_guard<std::mutex> acesso(mutexUsuarios);
    if (estaAguardandoChaveNFC()) {
        return vincularChaveNFC(uid, nomeUsuario);
    }

    const ResultadoLeituraNFC resultado =
        autenticarChaveNFC(uid, nomeUsuario, tipoAcesso, dataExpiracao);
    if (resultado == ResultadoLeituraNFC::AUTORIZADO) {
        liberarTranca();
    }
    return resultado;
}

void configurarRotasUsuarios(WebServer& server) {

    // ROTA POST /api/usuarios --------------------------------------

    server.on("/api/usuarios", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }

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

        std::unique_lock<std::mutex> acesso(mutexUsuarios);
        if (quantidadeUsuarios >= MAX_USUARIOS) {
            acesso.unlock();
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
            acesso.unlock();
            server.send(
                500,
                "application/json",
                "{\"erro\":\"Nao foi possivel persistir o usuario\"}"
            );
            return;
        }

        acesso.unlock();
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
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }
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

        std::unique_lock<std::mutex> acesso(mutexUsuarios);
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
                acesso.unlock();
                server.send(
                    500,
                    "application/json",
                    "{\"erro\":\"Nao foi possivel persistir as alteracoes do usuario\"}"
                );
                return;
            }

            acesso.unlock();
            JsonDocument resposta;
            resposta["mensagem"] = "Usuario atualizado com sucesso.";
            server.send(
                200,
                "application/json",
                respostaJson(resposta)
            );
            return;
        }

        acesso.unlock();
        server.send(
            404,
            "application/json",
            "{\"erro\":\"Usuario nao encontrado\"}"
        );
    });

    //ROTA GET /api/usuarios --------------------------------------

    server.on("/api/usuarios", HTTP_GET, [&server]() {
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }

        JsonDocument documento;
        JsonArray lista = documento.to<JsonArray>();
        const bool horarioConfiavel = horarioSistemaConfiavel();
        const time_t agora = time(nullptr);

        std::unique_lock<std::mutex> acesso(mutexUsuarios);
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

        acesso.unlock();
        server.send(
            200,
            "application/json",
            respostaJson(documento)
        );
    });

    server.on("/api/usuarios/desativar", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }

        if (!server.hasArg("id")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"ID do usuario e obrigatorio\"}"
            );
            return;
        }

        int id = server.arg("id").toInt();

        std::unique_lock<std::mutex> acesso(mutexUsuarios);
        for (int i = 0; i < quantidadeUsuarios; i++) {

            if (usuarios[i].id == id) {

                const bool ativoAnterior = usuarios[i].ativo;
                usuarios[i].ativo = false;
                if (!salvarUsuarios()) {
                    usuarios[i].ativo = ativoAnterior;
                    acesso.unlock();
                    server.send(500, "application/json",
                                "{\"erro\":\"Nao foi possivel salvar a desativacao\"}");
                    return;
                }

                if (usuarioAguardandoChave == id) {
                    usuarioAguardandoChave = -1;
                }

                const String nome = usuarios[i].nome;
                acesso.unlock();
                notificarUsuarioDesativado(nome);

                server.send(
                    200,
                    "application/json",
                    "{\"mensagem\":\"Usuario desativado com sucesso\"}"
                );

                return;
            }
        }

        acesso.unlock();
        server.send(
            404,
            "application/json",
            "{\"erro\":\"Usuario nao encontrado\"}"
        );
    });

    server.on("/api/usuarios", HTTP_DELETE, [&server]() {
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }

        if (!server.hasArg("id")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"ID do usuario e obrigatorio\"}"
            );
            return;
        }

        int id = server.arg("id").toInt();

        std::unique_lock<std::mutex> acesso(mutexUsuarios);
        for (int i = 0; i < quantidadeUsuarios; i++) {

            if (usuarios[i].id == id) {

                const Usuario usuarioRemovido = usuarios[i];

                // Move os usuarios seguintes uma posicao para tras
                for (int j = i; j < quantidadeUsuarios - 1; j++) {
                    usuarios[j] = usuarios[j + 1];
                }

                quantidadeUsuarios--;

                if (!salvarUsuarios()) {
                    for (int j = quantidadeUsuarios; j > i; j--) {
                        usuarios[j] = usuarios[j - 1];
                    }
                    usuarios[i] = usuarioRemovido;
                    quantidadeUsuarios++;
                    acesso.unlock();
                    server.send(500, "application/json",
                                "{\"erro\":\"Nao foi possivel salvar a exclusao\"}");
                    return;
                }

                usuarios[quantidadeUsuarios] = {};
                if (usuarioAguardandoChave == id) {
                    usuarioAguardandoChave = -1;
                }

                acesso.unlock();
                notificarUsuarioExcluido(usuarioRemovido.nome);

                server.send(
                    200,
                    "application/json",
                    "{\"mensagem\":\"Usuario excluido com sucesso\"}"
                );

                return;
            }
        }

        acesso.unlock();
        server.send(
            404,
            "application/json",
            "{\"erro\":\"Usuario nao encontrado\"}"
        );
    });

    server.on("/api/usuarios/aguardar-chave", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) {
            return;
        }

        if (!server.hasArg("id")) {
            server.send(
                400,
                "application/json",
                "{\"erro\":\"ID do usuario e obrigatorio\"}"
            );
            return;
        }

        int id = server.arg("id").toInt();

        std::unique_lock<std::mutex> acesso(mutexUsuarios);
        for (int i = 0; i < quantidadeUsuarios; i++) {

            if (usuarios[i].id == id) {

                if (!usuarios[i].ativo) {
                    acesso.unlock();
                    server.send(
                        400,
                        "application/json",
                        "{\"erro\":\"Usuario esta desativado\"}"
                    );
                    return;
                }

                if (usuarioAguardandoChave != id && estaAguardandoChaveNFC()) {
                    acesso.unlock();
                    server.send(409, "application/json",
                                "{\"erro\":\"Ja existe outra vinculacao NFC em andamento\"}");
                    return;
                }

                usuarioAguardandoChave = id;
                vinculacaoExpiraEm = esp_timer_get_time() + PRAZO_VINCULACAO_NFC_US;
                acesso.unlock();

                server.send(
                    200,
                    "application/json",
                    "{\"mensagem\":\"Aproxime a chave NFC do leitor em ate 30 segundos\"}"
                );

                return;
            }
        }

        acesso.unlock();
        server.send(
            404,
            "application/json",
            "{\"erro\":\"Usuario nao encontrado\"}"
        );
    });
}
