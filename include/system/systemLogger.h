#ifndef SYSTEM_LOGGER_H
#define SYSTEM_LOGGER_H

#include <Arduino.h>
#include <WebServer.h>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

bool inicializarLogs();
void registrarLog(LogLevel nivel, const String& mensagem);
bool adquirirAcessoLogs();
void liberarAcessoLogs();

#endif
