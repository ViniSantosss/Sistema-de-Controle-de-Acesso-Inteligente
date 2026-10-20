#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "telegram/telegramBot.h"
#include "secrets.h"

WiFiClientSecure client;

UniversalTelegramBot bot(
    TELEGRAM_BOT_TOKEN,
    client
);

void iniciarTelegram() {
    client.setInsecure();
}

bool enviarMensagemTelegram(const String& mensagem) {

    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    return bot.sendMessage(
        TELEGRAM_CHAT_ID,
        mensagem,
        ""
    );
}

void notificarUsuarioCadastrado(const String& nome) {

    String mensagem = "[AVISO] Novo usuario cadastrado: " + nome + ".";

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
        "[NEGADO] Acesso negado: chave NFC desconhecida (nao cadastrada).";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de chave NFC desconhecida.");
    }
}

void notificarAcessoNegadoUsuarioDesativado(const String& nome) {

    String mensagem =
        "[NEGADO] Acesso negado para " + nome +
        ": usuario desativado.";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de usuario desativado.");
    }
}

void notificarAcessoNegadoChaveExpirada(const String& nome) {

    String mensagem =
        "[NEGADO] Acesso negado para " + nome +
        ": chave NFC expirada.";

    if (!enviarMensagemTelegram(mensagem)) {
        Serial.println("Falha ao enviar notificacao de chave expirada.");
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