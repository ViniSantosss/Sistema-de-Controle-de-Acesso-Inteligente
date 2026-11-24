#include "tasks/core1Tasks.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "nfc/nfcManager.h"
#include "routes/userRoutes.h"
#include "system/systemLogger.h"
#include "telegram/telegramBot.h"

namespace {

void processarCartao(const String& uid) {
    String nomeUsuario;
    String tipoAcesso;
    String dataExpiracao;
    switch (processarLeituraNFC(uid, nomeUsuario, tipoAcesso, dataExpiracao)) {
        case ResultadoLeituraNFC::CHAVE_VINCULADA:
            registrarLog(LogLevel::INFO, "Chave NFC vinculada com sucesso.");
            notificarChaveNFCVinculada(nomeUsuario);
            break;
        case ResultadoLeituraNFC::CHAVE_JA_CADASTRADA:
            registrarLog(LogLevel::WARNING, "Chave NFC ja vinculada a outro usuario.");
            notificarChaveNFCDuplicada();
            break;
        case ResultadoLeituraNFC::VINCULACAO_FALHOU:
            registrarLog(LogLevel::ERROR, "Nao foi possivel vincular a chave NFC.");
            notificarFalhaVinculacaoChaveNFC();
            break;
        case ResultadoLeituraNFC::AUTORIZADO:
            registrarLog(LogLevel::INFO, "Acesso NFC autorizado.");
            notificarAcessoAutorizado(nomeUsuario, tipoAcesso);
            break;
        case ResultadoLeituraNFC::CHAVE_DESCONHECIDA:
            registrarLog(LogLevel::WARNING, "Acesso NFC negado: credencial desconhecida.");
            notificarAcessoNegadoChaveDesconhecida();
            break;
        case ResultadoLeituraNFC::USUARIO_DESATIVADO:
            registrarLog(LogLevel::WARNING, "Acesso NFC negado: usuario desativado.");
            notificarAcessoNegadoUsuarioDesativado(nomeUsuario);
            break;
        case ResultadoLeituraNFC::CHAVE_EXPIRADA:
            registrarLog(LogLevel::WARNING, "Acesso NFC negado: credencial expirada.");
            notificarAcessoNegadoChaveExpirada(nomeUsuario, dataExpiracao);
            break;
        case ResultadoLeituraNFC::VALIDADE_INDISPONIVEL:
            registrarLog(LogLevel::WARNING, "Acesso NFC negado: validade temporal nao verificavel.");
            notificarAcessoNegadoValidadeIndisponivel(nomeUsuario);
            break;
    }
}

void tarefaCore1(void*) {
    String uid;
    bool aguardandoRemocaoCartao = false;

    while (true) {
        const bool cartaoPresente = lerUID(uid);
        if (!cartaoPresente) {
            aguardandoRemocaoCartao = false;
        } else if (!aguardandoRemocaoCartao) {
            processarCartao(uid);
            aguardandoRemocaoCartao = true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}

void iniciarTasksCore1() {
    const BaseType_t resultado = xTaskCreatePinnedToCore(
        tarefaCore1,
        "Core1Task",
        4096,
        nullptr,
        1,
        nullptr,
        1
    );

    if (resultado != pdPASS) {
        Serial.println("Erro ao iniciar a tarefa de leitura NFC.");
        registrarLog(LogLevel::ERROR, "Falha ao iniciar a task de leitura NFC.");
    }
}
