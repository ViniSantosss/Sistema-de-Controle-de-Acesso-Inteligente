
#include "tasks/core0Tasks.h"
#include "hardware/hardwarePins.h"
#include "sensors/doorHistory.h"
#include "time/systemClock.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

// verifica se a porta está aberta ou fechada, com base no sensor magnético
static volatile bool estadoPortaAberta = false;

// looping que verifica se a porta está aberta ou fechada
static constexpr int LEITURAS_PARA_CONFIRMAR = 5;
static constexpr TickType_t INTERVALO_LEITURA =
    pdMS_TO_TICKS(10);

// retorna true se a porta estiver aberta, false caso contrário
bool portaEstaAberta() {
    return estadoPortaAberta;
}

// task que roda no core 0, responsável por ler o estado do sensor magnético da porta
static void tarefaCore0(void* parametro) {
    pinMode(PIN_SENSOR_PORTA, INPUT_PULLUP);

    bool leituraAnterior = digitalRead(PIN_SENSOR_PORTA);
    int leiturasIguais = 0;
    bool estadoConfirmado = false;
    bool estadoRegistrado = false;
    bool avisoFilaCheia = false;

    while (true) {
        bool leituraAtual = digitalRead(PIN_SENSOR_PORTA);

        if (leituraAtual == leituraAnterior) {
            leiturasIguais++;
        } else {
            leituraAnterior = leituraAtual;
            leiturasIguais = 1;
        }

        // isso aqui serve para evitar que o estado da porta mude rapidamente, tipo se ela estiver meio aberta ou fechada, e o sensor ficar oscilando entre HIGH e LOW. A gente só considera que a porta mudou de estado se a leitura for consistente por um certo número de vezes.
        if (leiturasIguais >= LEITURAS_PARA_CONFIRMAR) {
            const bool novoEstado = leituraAtual == HIGH;
            if (!estadoConfirmado || estadoPortaAberta != novoEstado) {
                estadoPortaAberta = novoEstado;
                estadoConfirmado = true;
                estadoRegistrado = false;
            }

            if (!estadoRegistrado && horarioSistemaConfiavel()) {
                const int64_t timestamp = static_cast<int64_t>(time(nullptr));
                if (enfileirarEstadoPorta(estadoPortaAberta, timestamp)) {
                    estadoRegistrado = true;
                    avisoFilaCheia = false;
                } else if (!avisoFilaCheia) {
                    Serial.println(
                        "Historico da porta: fila cheia; leitura aguardando."
                    );
                    avisoFilaCheia = true;
                }
            }
        }

        // trava a task por um tempo e depois volta a ler o estado do sensor magnético da porta
        vTaskDelay(INTERVALO_LEITURA);
    }
}

// cria a task no core 0, que vai rodar a função tarefaCore0
void iniciarCore0Task() {
    const BaseType_t resultado = xTaskCreatePinnedToCore(
        tarefaCore0,
        "Core0Task",
        4096,
        nullptr,
        1,
        nullptr,
        0
    );

    if (resultado != pdPASS) {
        Serial.println("Erro ao iniciar a tarefa do sensor da porta.");
    }
}
