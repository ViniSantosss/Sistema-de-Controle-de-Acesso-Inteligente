#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <cstring>
#include <cstdlib>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "telegram/telegramBot.h"
#include "secrets.h"
#include "time/systemClock.h"

static constexpr UBaseType_t TAMANHO_FILA_TELEGRAM = 8;
static QueueHandle_t filaMensagensTelegram = nullptr;

WiFiClientSecure client;

UniversalTelegramBot bot(
    TELEGRAM_BOT_TOKEN,
    client
);

static void tarefaEnvioTelegram(void* parametro) {
    (void)parametro;

    char* mensagem = nullptr;

    while (true) {
        if (mensagem == nullptr) {
            if (xQueueReceive(
                    filaMensagensTelegram,
                    &mensagem,
                    pdMS_TO_TICKS(500)
                ) != pdTRUE) {
                continue;
            }
        }

        if (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (!bot.sendMessage(TELEGRAM_CHAT_ID, mensagem, "")) {
            Serial.println("Falha ao enviar notificacao pelo Telegram.");
        }

        free(mensagem);
        mensagem = nullptr;
    }
}

void iniciarTelegram() {
    client.setInsecure();
    client.setHandshakeTimeout(5);
    client.setTimeout(5);

    if (filaMensagensTelegram != nullptr) {
        return;
    }

    filaMensagensTelegram = xQueueCreate(
        TAMANHO_FILA_TELEGRAM,
        sizeof(char*)
    );

    if (filaMensagensTelegram == nullptr) {
        Serial.println("Erro ao criar a fila de mensagens do Telegram.");
        return;
    }

    if (xTaskCreatePinnedToCore(
            tarefaEnvioTelegram,
            "TelegramTask",
            8192,
            nullptr,
            1,
            nullptr,
            1
        ) != pdPASS) {
        vQueueDelete(filaMensagensTelegram);
        filaMensagensTelegram = nullptr;
        Serial.println("Erro ao iniciar a tarefa do Telegram.");
    }
}

bool enviarMensagemTelegram(const String& mensagem) {
    if (filaMensagensTelegram == nullptr) {
        Serial.println("Fila do Telegram indisponivel; notificacao descartada.");
        return false;
    }

    char* copiaMensagem = static_cast<char*>(malloc(mensagem.length() + 1));

    if (copiaMensagem == nullptr) {
        Serial.println("Sem memoria para enfileirar notificacao do Telegram.");
        return false;
    }

    memcpy(copiaMensagem, mensagem.c_str(), mensagem.length() + 1);

    if (xQueueSend(filaMensagensTelegram, &copiaMensagem, 0) != pdTRUE) {
        free(copiaMensagem);
        Serial.println("Fila do Telegram cheia; notificacao descartada.");
        return false;
    }

    return true;
}

void notificarUsuarioCadastrado(
    const String& nome,
    const String& tipoAcesso
) {

    String mensagem =
        "[AVISO] Usuario " + nome + " criado. Tipo de acesso: " +
        tipoAcesso + ".";

    enviarMensagemTelegram(mensagem);
}

void notificarUsuarioDesativado(const String& nome) {

    String mensagem = "[AVISO] Usuario desativado: " + nome + ".";

    enviarMensagemTelegram(mensagem);
}

void notificarAcessoAutorizado(
    const String& nome,
    const String& tipoAcesso
) {

    String mensagem =
        "[OK] Tentativa de acesso autorizada para " + nome +
        " (tipo: " + tipoAcesso + ").";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de acesso autorizado.");
    }
}

void notificarAcessoNegadoChaveDesconhecida() {

    String mensagem =
        "[ERRO AO ENTRAR] Cartao nao identificado: credencial NFC nao cadastrada.";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de chave NFC desconhecida.");
    }
}

void notificarAcessoNegadoUsuarioDesativado(const String& nome) {

    String mensagem =
        "[ERRO AO ENTRAR] Acesso negado para " + nome +
        ": usuario desativado.";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de usuario desativado.");
    }
}

void notificarAcessoNegadoChaveExpirada(
    const String& nome,
    const String& dataExpiracao
) {

    String mensagem =
        "[ERRO AO ENTRAR] A chave do usuario " + nome +
        " expirou em " +
        formatarDataExpiracao(dataExpiracao) +
        " (horario de Sao Paulo).";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de acesso expirado.");
    }
}

void notificarAcessoNegadoValidadeIndisponivel(const String& nome) {
    String mensagem =
        "[ERRO AO ENTRAR] Acesso negado para " + nome +
        ": nao foi possivel verificar a validade temporal. "
        "Confirme a sincronizacao de horario por NTP e a data de expiracao.";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de validade indisponivel.");
    }
}

void notificarUsuarioExcluido(const String& nome) {

    String mensagem =
        "[AVISO] Usuario excluido: " + nome + ".";

    enviarMensagemTelegram(mensagem);
}

void notificarChaveNFCVinculada(const String& nome) {

    String mensagem =
        "[OK] Chave vinculada com sucesso: Chave NFC cadastrada para " +
        nome + ".";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de vinculacao NFC.");
    }
}

void notificarChaveNFCDuplicada() {

    if (!enviarMensagemTelegram(
            "[NEGADO] Chave ja cadastrada: Esta chave NFC ja esta vinculada a outro usuario."
        )) {
        Serial.println("Falha ao enviar notificacao de chave NFC duplicada.");
    }
}

void notificarFalhaVinculacaoChaveNFC() {

    if (!enviarMensagemTelegram(
            "[ERRO] Falha na vinculacao: Nao foi possivel cadastrar a chave NFC."
        )) {
        Serial.println("Falha ao enviar notificacao de falha na vinculacao NFC.");
    }
}