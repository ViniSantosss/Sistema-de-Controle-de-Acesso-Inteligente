#ifndef USER_ROUTES_H
#define USER_ROUTES_H

#include <WebServer.h>

void configurarRotasUsuarios(WebServer& server);

enum class ResultadoVinculacaoNFC {
    SUCESSO,
    CHAVE_JA_CADASTRADA,
    FALHA
};

enum class ResultadoAutenticacaoNFC {
    AUTORIZADO,
    CHAVE_DESCONHECIDA,
    USUARIO_DESATIVADO,
    CHAVE_EXPIRADA
};

ResultadoVinculacaoNFC vincularChaveNFC(const String& uid, String& nomeUsuario);
ResultadoAutenticacaoNFC autenticarChaveNFC(
    const String& uid,
    String& nomeUsuario,
    String& tipoAcesso
);
int getUsuarioAguardandoChave();
void finalizarAguardandoChaveNFC();

bool estaAguardandoChaveNFC();

#endif