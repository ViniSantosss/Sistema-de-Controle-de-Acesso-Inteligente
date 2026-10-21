#include "routes/deepSleepRoutes.h"

#include <Arduino.h>
#include <esp_sleep.h>

void configurarRotasDeepSleep(WebServer& server) {

    server.on("/api/deep-sleep", HTTP_POST, [&server]() {
        Serial.println("Ativando Deep Sleep...");

        server.send(
            200,
            "application/json",
            "{\"mensagem\":\"ESP32 entrando em Deep Sleep\"}"
        );

        Serial.println("Deep Sleep Ativado.");
        Serial.flush();
        delay(100);

        esp_deep_sleep_start();
    });
}