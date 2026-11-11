#ifndef USER_ROUTES_H
#define USER_ROUTES_H

#include <WebServer.h>

void configurarRotasUsuarios(WebServer& server);

enum class ResultadoLeituraNFC {
    CHAVE_VINCULADA,
    CHAVE_JA_CADASTRADA,
    VINCULACAO_FALHOU,
    AUTORIZADO,
    CHAVE_DESCONHECIDA,
    USUARIO_DESATIVADO,
    CHAVE_EXPIRADA,
    VALIDADE_INDISPONIVEL
};

ResultadoLeituraNFC processarLeituraNFC(
    const String& uid,
    String& nomeUsuario,
    String& tipoAcesso,
    String& dataExpiracao
);
#endif
