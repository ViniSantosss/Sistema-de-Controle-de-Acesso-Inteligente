#ifndef USER_H
#define USER_H

#include <Arduino.h>

constexpr int MAX_USUARIOS = 20;

struct Usuario {
    int id;
    String nome;
    String tipoAcesso;
    bool ativo;
    String credencialNFC;
    String dataExpiracao;
};

#endif