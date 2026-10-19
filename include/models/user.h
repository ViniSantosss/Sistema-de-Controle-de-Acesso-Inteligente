#ifndef USER_H
#define USER_H

#include <Arduino.h>

struct Usuario {
    int id;
    String nome;
    String tipoAcesso;
    bool ativo;
    String credencialNFC;
};

#endif