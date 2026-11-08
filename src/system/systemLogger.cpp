#include "system/systemLogger.h"

#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <time.h>

#include "time/systemClock.h"

namespace {

constexpr char ARQUIVO_LOGS[] = "/logs.txt";
constexpr char ARQUIVO_TEMPORARIO[] = "/logs.tmp";
constexpr char ARQUIVO_BACKUP[] = "/logs.bak";
constexpr size_t MAX_REGISTROS_LOG = 1000;
constexpr size_t MAX_TAMANHO_MENSAGEM = 160;

SemaphoreHandle_t mutexLogs = nullptr;
size_t quantidadeRegistros = 0;
bool logsDisponiveis = false;

const char* nomeNivel(LogLevel nivel) {
    switch (nivel) {
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
    }

    return "INFO";
}

String criarLinhaLog(LogLevel nivel, const String& mensagem) {
    String linha;
    if (horarioSistemaConfiavel()) {
        const time_t agora = time(nullptr);
        struct tm horario = {};
        char dataHora[20];
        if (localtime_r(&agora, &horario) != nullptr &&
            strftime(dataHora, sizeof(dataHora), "%Y-%m-%d %H:%M:%S", &horario) > 0) {
            linha = dataHora;
        } else {
            linha = "SEM_HORARIO";
        }
    } else {
        linha = "SEM_HORARIO uptime=" + String(millis()) + "ms";
    }

    linha += " [";
    linha += nomeNivel(nivel);
    linha += "] ";

    for (size_t i = 0; i < mensagem.length() && i < MAX_TAMANHO_MENSAGEM; i++) {
        const char caractere = mensagem[i];
        linha += caractere == '\r' || caractere == '\n' ? ' ' : caractere;
    }

    linha += '\n';
    return linha;
}

bool compactarLogs(size_t manterRegistros) {
    File origem = LittleFS.open(ARQUIVO_LOGS, "r");
    if (!origem) {
        return false;
    }

    const size_t pular = quantidadeRegistros > manterRegistros
        ? quantidadeRegistros - manterRegistros
        : 0;
    File temporario = LittleFS.open(ARQUIVO_TEMPORARIO, "w");
    if (!temporario) {
        origem.close();
        return false;
    }

    size_t indice = 0;
    bool gravacaoCompleta = true;
    while (origem.available()) {
        String linha = origem.readStringUntil('\n');
        if (indice++ < pular) {
            continue;
        }

        linha += '\n';
        if (temporario.print(linha) != linha.length()) {
            gravacaoCompleta = false;
            break;
        }
    }

    origem.close();
    temporario.close();

    if (!gravacaoCompleta) {
        LittleFS.remove(ARQUIVO_TEMPORARIO);
        return false;
    }

    LittleFS.remove(ARQUIVO_BACKUP);
    if (!LittleFS.rename(ARQUIVO_LOGS, ARQUIVO_BACKUP)) {
        LittleFS.remove(ARQUIVO_TEMPORARIO);
        return false;
    }

    if (!LittleFS.rename(ARQUIVO_TEMPORARIO, ARQUIVO_LOGS)) {
        LittleFS.rename(ARQUIVO_BACKUP, ARQUIVO_LOGS);
        LittleFS.remove(ARQUIVO_TEMPORARIO);
        return false;
    }

    LittleFS.remove(ARQUIVO_BACKUP);
    quantidadeRegistros = manterRegistros;
    return true;
}

}

bool inicializarLogs() {
    if (mutexLogs == nullptr) {
        mutexLogs = xSemaphoreCreateMutex();
    }
    if (mutexLogs == nullptr) {
        Serial.println("Erro ao criar mutex do sistema de logs.");
        return false;
    }

    if (!LittleFS.exists(ARQUIVO_LOGS)) {
        File arquivo = LittleFS.open(ARQUIVO_LOGS, "w");
        if (!arquivo) {
            Serial.println("Erro ao criar arquivo de logs.");
            return false;
        }
        arquivo.close();
    }

    File arquivo = LittleFS.open(ARQUIVO_LOGS, "r");
    if (!arquivo) {
        Serial.println("Erro ao abrir arquivo de logs.");
        return false;
    }

    quantidadeRegistros = 0;
    while (arquivo.available()) {
        arquivo.readStringUntil('\n');
        quantidadeRegistros++;
    }
    arquivo.close();

    logsDisponiveis = true;
    if (quantidadeRegistros > MAX_REGISTROS_LOG &&
        !compactarLogs(MAX_REGISTROS_LOG)) {
        logsDisponiveis = false;
        Serial.println("Erro ao limitar o arquivo aos ultimos 1000 logs.");
        return false;
    }

    return true;
}

void registrarLog(LogLevel nivel, const String& mensagem) {
    const String linha = criarLinhaLog(nivel, mensagem);
    Serial.print(linha);

    if (!logsDisponiveis || mutexLogs == nullptr ||
        xSemaphoreTake(mutexLogs, portMAX_DELAY) != pdTRUE) {
        Serial.println("Erro: log nao persistido na flash.");
        return;
    }

    if (quantidadeRegistros >= MAX_REGISTROS_LOG &&
        !compactarLogs(MAX_REGISTROS_LOG - 1)) {
        xSemaphoreGive(mutexLogs);
        Serial.println("Erro: nao foi possivel liberar espaco para o log.");
        return;
    }

    File arquivo = LittleFS.open(ARQUIVO_LOGS, "a");
    if (!arquivo) {
        xSemaphoreGive(mutexLogs);
        Serial.println("Erro: nao foi possivel abrir o arquivo de logs.");
        return;
    }

    const size_t bytesGravados = arquivo.print(linha);
    arquivo.close();
    if (bytesGravados != linha.length()) {
        xSemaphoreGive(mutexLogs);
        Serial.println("Erro: gravacao incompleta no arquivo de logs.");
        return;
    }

    quantidadeRegistros++;
    xSemaphoreGive(mutexLogs);
}

bool adquirirAcessoLogs() {
    return logsDisponiveis &&
        mutexLogs != nullptr &&
        xSemaphoreTake(mutexLogs, portMAX_DELAY) == pdTRUE;
}

void liberarAcessoLogs() {
    if (mutexLogs != nullptr) {
        xSemaphoreGive(mutexLogs);
    }
}
