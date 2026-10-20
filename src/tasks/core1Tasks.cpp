#include "tasks/core1Tasks.h"

#include "nfc/nfcManager.h"
#include "routes/userRoutes.h"
#include "telegram/telegramBot.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void tarefaCore1(void* parametro) {

    String uid;

    while (true) {

        if (lerUID(uid)) {

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
                ResultadoAutenticacaoNFC resultado =
                    autenticarChaveNFC(uid, nomeUsuario, tipoAcesso);

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
                        notificarAcessoNegadoChaveExpirada(nomeUsuario);
                        break;
                }
            }

            while (lerUID(uid)) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void iniciarCore1Task() {

    xTaskCreatePinnedToCore(
        tarefaCore1,
        "Core1Task",
        4096,
        nullptr,
        1,
        nullptr,
        1
    );
}