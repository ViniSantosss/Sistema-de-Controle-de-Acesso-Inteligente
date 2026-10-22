#include "sensors/doorHistory.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "time/systemClock.h"

namespace {

constexpr char ARQUIVO_HISTORICO[] = "/historico_porta.json";
constexpr char ARQUIVO_TEMPORARIO[] = "/historico_porta.tmp";
constexpr size_t MAX_EVENTOS_HISTORICO = 512;
constexpr int64_t JANELA_HISTORICO_SEGUNDOS = 24 * 60 * 60;

struct EventoPorta {
    int64_t timestamp;
    bool portaAberta;
};

EventoPorta eventos[MAX_EVENTOS_HISTORICO];
size_t quantidadeEventos = 0;
int64_t historicoIncompletoAte = 0;
QueueHandle_t filaEventos = nullptr;
bool historicoDisponivel = false;

void removerPrimeiroEvento() {
    if (quantidadeEventos == 0) {
        return;
    }

    for (size_t i = 1; i < quantidadeEventos; i++) {
        eventos[i - 1] = eventos[i];
    }
    quantidadeEventos--;
}

bool persistirHistorico() {
    JsonDocument documento;
    documento["incompletoAte"] = historicoIncompletoAte;
    JsonArray lista = documento["eventos"].to<JsonArray>();

    for (size_t i = 0; i < quantidadeEventos; i++) {
        JsonObject evento = lista.add<JsonObject>();
        evento["timestamp"] = eventos[i].timestamp;
        evento["portaAberta"] = eventos[i].portaAberta;
    }

    File arquivo = LittleFS.open(ARQUIVO_TEMPORARIO, "w");
    if (!arquivo) {
        Serial.println("Historico da porta: erro ao criar arquivo temporario.");
        return false;
    }

    const size_t bytesEsperados = measureJson(documento);
    const size_t bytesGravados = serializeJson(documento, arquivo);
    arquivo.close();

    if (bytesGravados != bytesEsperados) {
        LittleFS.remove(ARQUIVO_TEMPORARIO);
        Serial.println("Historico da porta: gravacao incompleta.");
        return false;
    }

    if (!LittleFS.rename(ARQUIVO_TEMPORARIO, ARQUIVO_HISTORICO)) {
        LittleFS.remove(ARQUIVO_TEMPORARIO);
        Serial.println("Historico da porta: erro ao substituir o arquivo salvo.");
        return false;
    }

    return true;
}

bool carregarHistorico() {
    if (!LittleFS.exists(ARQUIVO_HISTORICO)) {
        return true;
    }

    File arquivo = LittleFS.open(ARQUIVO_HISTORICO, "r");
    if (!arquivo) {
        Serial.println("Historico da porta: erro ao abrir o arquivo salvo.");
        return false;
    }

    JsonDocument documento;
    const DeserializationError erro = deserializeJson(documento, arquivo);
    arquivo.close();

    if (erro || !documento["eventos"].is<JsonArray>()) {
        Serial.println("Historico da porta: arquivo salvo com formato invalido.");
        return false;
    }

    historicoIncompletoAte = documento["incompletoAte"] | 0LL;
    for (JsonObjectConst evento : documento["eventos"].as<JsonArrayConst>()) {
        const int64_t timestamp = evento["timestamp"] | 0LL;
        if (timestamp <= 0 || !evento["portaAberta"].is<bool>()) {
            Serial.println("Historico da porta: registro invalido ignorado.");
            continue;
        }

        if (quantidadeEventos == MAX_EVENTOS_HISTORICO) {
            const int64_t removidoAte =
                eventos[0].timestamp + JANELA_HISTORICO_SEGUNDOS;
            removerPrimeiroEvento();
            if (removidoAte > historicoIncompletoAte) {
                historicoIncompletoAte = removidoAte;
            }
        }

        eventos[quantidadeEventos++] = {
            timestamp,
            evento["portaAberta"].as<bool>()
        };
    }

    return true;
}

bool removerEventosForaDaJanela(int64_t agora) {
    if (agora <= 0) {
        return false;
    }

    const int64_t limite = agora - JANELA_HISTORICO_SEGUNDOS;
    bool alterado = false;

    // Preserva a última leitura anterior ao limite para conhecer o estado no início da janela.
    while (quantidadeEventos > 1 && eventos[1].timestamp < limite) {
        removerPrimeiroEvento();
        alterado = true;
    }

    return alterado;
}

}

bool inicializarHistoricoPorta() {
    if (filaEventos != nullptr) {
        return historicoDisponivel;
    }

    filaEventos = xQueueCreate(8, sizeof(EventoPorta));
    if (filaEventos == nullptr) {
        Serial.println("Historico da porta: erro ao criar fila de eventos.");
        return false;
    }

    historicoDisponivel = carregarHistorico();
    return historicoDisponivel;
}

bool enfileirarEstadoPorta(bool portaAberta, int64_t timestamp) {
    if (!historicoDisponivel || filaEventos == nullptr || timestamp <= 0) {
        return false;
    }

    const EventoPorta evento = { timestamp, portaAberta };
    return xQueueSend(filaEventos, &evento, 0) == pdTRUE;
}

void processarHistoricoPortaPendente() {
    if (!historicoDisponivel || filaEventos == nullptr) {
        return;
    }

    bool alterado = false;
    EventoPorta evento;

    while (xQueueReceive(filaEventos, &evento, 0) == pdTRUE) {
        if (quantidadeEventos == MAX_EVENTOS_HISTORICO) {
            const int64_t removidoAte =
                eventos[0].timestamp + JANELA_HISTORICO_SEGUNDOS;
            if (removidoAte > historicoIncompletoAte) {
                historicoIncompletoAte = removidoAte;
            }
            removerPrimeiroEvento();
        }

        eventos[quantidadeEventos++] = evento;
        alterado = true;
    }

    if (horarioSistemaConfiavel()) {
        const int64_t agora = static_cast<int64_t>(time(nullptr));
        if (removerEventosForaDaJanela(agora)) {
            alterado = true;
        }

        if (historicoIncompletoAte <= agora &&
            historicoIncompletoAte != 0) {
            historicoIncompletoAte = 0;
            alterado = true;
        }
    }

    if (alterado && !persistirHistorico()) {
        Serial.println(
            "Historico da porta: leitura mantida em RAM, mas nao persistida."
        );
    }
}

String obterHistoricoPortaJson() {
    JsonDocument documento;
    documento["disponivel"] = historicoDisponivel;
    const bool horarioConfiavel = horarioSistemaConfiavel();
    documento["horarioConfiavel"] = horarioConfiavel;

    if (!historicoDisponivel || !horarioConfiavel) {
        documento["completo"] = false;
        documento["agora"] = nullptr;
        documento["eventos"].to<JsonArray>();
    } else {
        const int64_t agora = static_cast<int64_t>(time(nullptr));
        if (removerEventosForaDaJanela(agora) &&
            !persistirHistorico()) {
            Serial.println(
                "Historico da porta: falha ao persistir a limpeza da janela."
            );
        }
        documento["agora"] = agora;
        documento["completo"] =
            historicoIncompletoAte == 0 ||
            historicoIncompletoAte <= agora;

        JsonArray lista = documento["eventos"].to<JsonArray>();
        for (size_t i = 0; i < quantidadeEventos; i++) {
            JsonObject evento = lista.add<JsonObject>();
            evento["timestamp"] = eventos[i].timestamp;
            evento["portaAberta"] = eventos[i].portaAberta;
        }
    }

    String json;
    serializeJson(documento, json);
    return json;
}
