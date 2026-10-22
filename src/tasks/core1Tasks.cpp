#include "tasks/core1Tasks.h"

#include "nfc/nfcManager.h"
#include "routes/userRoutes.h"
#include "telegram/telegramBot.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void tarefaCore1(void* parametro) {

    String uid;
    bool aguardandoRemocaoCartao = false;

    while (true) {
        if (aguardandoRemocaoCartao) {
            if (!lerUID(uid)) {
                aguardandoRemocaoCartao = false;
            }
        } else if (lerUID(uid)) {

            Serial.print("UID detectado: ");
            Serial.println(uid);

            if (estaAguardandoChaveNFC()) {
                String nomeUsuario;
                ResultadoVinculacaoNFC resultado =
                    vincularChaveNFC(uid, nomeUsuario);

                switch (resultado) {
                    case ResultadoVinculacaoNFC::SUCESSO:
                        Serial.println("Chave NFC vinculada com sucesso.");
                        notificarChaveNFCVinculada(nomeUsuario);
                        break;
                    case ResultadoVinculacaoNFC::CHAVE_JA_CADASTRADA:
                        Serial.println("Chave NFC ja vinculada a outro usuario.");
                        notificarChaveNFCDuplicada();
                        break;
                    case ResultadoVinculacaoNFC::FALHA:
                        Serial.println("Nao foi possivel vincular a chave NFC.");
                        notificarFalhaVinculacaoChaveNFC();
                        break;
                }

            } else {
                String nomeUsuario;
                String tipoAcesso;
                String dataExpiracao;
                ResultadoAutenticacaoNFC resultado =
                    autenticarChaveNFC(
                        uid,
                        nomeUsuario,
                        tipoAcesso,
                        dataExpiracao
                    );

                switch (resultado) {
                    case ResultadoAutenticacaoNFC::AUTORIZADO:
                        notificarAcessoAutorizado(nomeUsuario, tipoAcesso);
                        break;
                    case ResultadoAutenticacaoNFC::CHAVE_DESCONHECIDA:
                        notificarAcessoNegadoChaveDesconhecida();
                        break;
                    case ResultadoAutenticacaoNFC::USUARIO_DESATIVADO:
                        notificarAcessoNegadoUsuarioDesativado(nomeUsuario);
                        break;
                    case ResultadoAutenticacaoNFC::CHAVE_EXPIRADA:
                        notificarAcessoNegadoChaveExpirada(
                            nomeUsuario,
                            dataExpiracao
                        );
                        break;
                    case ResultadoAutenticacaoNFC::VALIDADE_INDISPONIVEL:
                        notificarAcessoNegadoValidadeIndisponivel(nomeUsuario);
                        break;
                }
            }

            aguardandoRemocaoCartao = true;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void iniciarCore1Task() {

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
    }
}