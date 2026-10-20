#include "routes/pageRoutes.h"

#include <LittleFS.h>

void configurarRotasPaginas(WebServer& server) {

    server.on("/", HTTP_GET, [&server]() {
        File file = LittleFS.open("/index.html", "r");

        if (!file) {
            server.send(404, "text/plain", "index.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/panel.html", HTTP_GET, [&server]() {
        File file = LittleFS.open("/panel.html", "r");

        if (!file) {
            server.send(404, "text/plain", "panel.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/users.html", HTTP_GET, [&server]() {
        File file = LittleFS.open("/users.html", "r");

        if (!file) {
            server.send(404, "text/plain", "users.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/config.html", HTTP_GET, [&server]() {
        File file = LittleFS.open("/config.html", "r");

        if (!file) {
            server.send(404, "text/plain", "config.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/performance.html", HTTP_GET, [&server]() {
        File file = LittleFS.open("/performance.html", "r");

        if (!file) {
            server.send(404, "text/plain", "performance.html nao encontrado");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/css/style.css", HTTP_GET, [&server]() {
    File file = LittleFS.open("/css/style.css", "r");

    if (!file) {
        server.send(404, "text/plain", "style.css nao encontrado");
        return;
    }

    server.streamFile(file, "text/css");
        file.close();
    });

    server.on("/js/app.js", HTTP_GET, [&server]() {
        File file = LittleFS.open("/js/app.js", "r");

        if (!file) {
            server.send(404, "text/plain", "app.js nao encontrado");
            return;
        }

        server.streamFile(file, "application/javascript");
        file.close();
    });

    server.on("/icons.svg", HTTP_GET, [&server]() {
        File file = LittleFS.open("/icons.svg", "image/svg+xml");

        if (!file) {
            server.send(404, "text/plain", "icons.svg nao encontrado");
            return;
        }

        server.streamFile(file, "image/svg+xml");
        file.close();
    });
}