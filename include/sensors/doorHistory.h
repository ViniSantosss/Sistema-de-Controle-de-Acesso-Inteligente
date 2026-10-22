#ifndef DOOR_HISTORY_H
#define DOOR_HISTORY_H

#include <Arduino.h>

bool inicializarHistoricoPorta();
bool enfileirarEstadoPorta(bool portaAberta, int64_t timestamp);
void processarHistoricoPortaPendente();
String obterHistoricoPortaJson();

#endif
