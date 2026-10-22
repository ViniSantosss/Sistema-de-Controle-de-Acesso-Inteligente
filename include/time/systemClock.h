#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <Arduino.h>
#include <time.h>

void iniciarRelogioSistema();
bool horarioSistemaConfiavel();
bool interpretarDataExpiracao(const String& dataHora, time_t& instante);
String formatarDataExpiracao(const String& dataHora);

#endif
