#ifndef AUTH_ROUTES_H
#define AUTH_ROUTES_H

#include <WebServer.h>

void configurarRotasAuth(WebServer& server);
bool administradorAutenticado(WebServer& server);

#endif