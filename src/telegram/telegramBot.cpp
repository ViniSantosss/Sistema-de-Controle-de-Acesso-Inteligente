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

    String mensagem = "🔔 Novo usuario cadastrado: " + nome + " 🔔";

    enviarMensagemTelegram(mensagem);
}

void notificarUsuarioDesativado(const String& nome) {

    String mensagem = "🔔 Usuario desativado: " + nome + " 🔔";

    enviarMensagemTelegram(mensagem);
}

void notificarAcessoAutorizado(const String& nome) {

    String mensagem = "✅ Acesso autorizado para: " + nome + " ✅";

    enviarMensagemTelegram(mensagem);
}

void notificarAcessoNegado(const String& nome) {

    String mensagem = "❌ Acesso negado para: " + nome + " ❌";

    enviarMensagemTelegram(mensagem);
}

void notificarUsuarioExcluido(const String& nome) {

    String mensagem = "❌ Usuario excluido: " + nome + " ❌";

    enviarMensagemTelegram(mensagem);
}