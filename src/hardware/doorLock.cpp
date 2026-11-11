#include "hardware/doorLock.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <mutex>

#include "hardware/hardwarePins.h"
#include "secrets.h"

#ifdef RELE_TRANCA_NIVEL_ATIVO
namespace {

static_assert(RELE_TRANCA_NIVEL_ATIVO == LOW || RELE_TRANCA_NIVEL_ATIVO == HIGH,
              "O nivel ativo do rele precisa ser LOW ou HIGH.");
constexpr auto PINO_TRANCA = static_cast<gpio_num_t>(PIN_RELE_TRANCA);
constexpr int NIVEL_INATIVO = RELE_TRANCA_NIVEL_ATIVO == LOW ? HIGH : LOW;
constexpr uint64_t PULSO_TRANCA_US = 3000000;
esp_timer_handle_t temporizador = nullptr;
std::mutex mutexTranca;
bool trancaHabilitada = false;
bool pulsoEmAndamento = false;

void encerrarPulso(void*) {
    const std::lock_guard<std::mutex> acesso(mutexTranca);
    gpio_set_level(PINO_TRANCA, NIVEL_INATIVO);
    pulsoEmAndamento = false;
}

}
#endif

void inicializarTranca() {
#ifdef RELE_TRANCA_NIVEL_ATIVO
    gpio_set_level(PINO_TRANCA, NIVEL_INATIVO);
    if (gpio_set_direction(PINO_TRANCA, GPIO_MODE_OUTPUT) != ESP_OK) {
        Serial.println("Falha ao configurar o GPIO da tranca.");
        return;
    }

    esp_timer_create_args_t configuracao = {};
    configuracao.callback = encerrarPulso;
    configuracao.name = "pulso_tranca";
    if (esp_timer_create(&configuracao, &temporizador) != ESP_OK) {
        Serial.println("Falha ao inicializar o temporizador da tranca.");
        return;
    }
    trancaHabilitada = true;
#endif
}

void liberarTranca() {
#ifdef RELE_TRANCA_NIVEL_ATIVO
    const std::lock_guard<std::mutex> acesso(mutexTranca);
    if (!trancaHabilitada || pulsoEmAndamento) {
        return;
    }
    if (esp_timer_start_once(temporizador, PULSO_TRANCA_US) != ESP_OK) {
        Serial.println("Falha ao iniciar o pulso da tranca.");
        return;
    }
    gpio_set_level(PINO_TRANCA, RELE_TRANCA_NIVEL_ATIVO);
    pulsoEmAndamento = true;
#endif
}

void desenergizarTranca() {
#ifdef RELE_TRANCA_NIVEL_ATIVO
    const std::lock_guard<std::mutex> acesso(mutexTranca);
    trancaHabilitada = false;
    gpio_set_level(PINO_TRANCA, NIVEL_INATIVO);
#endif
}
