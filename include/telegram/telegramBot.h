#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

void iniciarTelegram();

bool enviarMensagemTelegram(const String& mensagem);

void notificarUsuarioCadastrado(const String& nome);
void notificarUsuarioDesativado(const String& nome);
void notificarAcessoAutorizado(const String& nome);
void notificarAcessoNegado(const String& nome);

#endif