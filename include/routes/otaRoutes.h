#ifndef OTA_ROUTES_H
#define OTA_ROUTES_H

#include <WebServer.h>

void configurarRotasOta(WebServer& server);
void processarReinicioOtaPendente();

#endif
