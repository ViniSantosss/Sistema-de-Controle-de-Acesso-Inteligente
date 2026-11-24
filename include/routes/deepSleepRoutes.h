#ifndef DEEP_SLEEP_ROUTES_H
#define DEEP_SLEEP_ROUTES_H

#include <WebServer.h>

void verificarDespertarDeepSleep();
void configurarRotasDeepSleep(WebServer& server);
void processarDeepSleepPendente();

#endif