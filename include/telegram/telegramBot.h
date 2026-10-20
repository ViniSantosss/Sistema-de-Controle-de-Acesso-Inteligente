#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

void iniciarTelegram();

bool enviarMensagemTelegram(const String& mensagem);

void notificarUsuarioCadastrado(const String& nome);
void notificarUsuarioDesativado(const String& nome);
void notificarAcessoAutorizado(const String& nome, const String& tipoAcesso);
void notificarAcessoNegadoChaveDesconhecida();
void notificarAcessoNegadoUsuarioDesativado(const String& nome);
void notificarAcessoNegadoChaveExpirada(const String& nome);
void notificarUsuarioExcluido(const String& nome);
void notificarChaveNFCVinculada(const String& nome);
void notificarChaveNFCDuplicada();
void notificarFalhaVinculacaoChaveNFC();

#endif