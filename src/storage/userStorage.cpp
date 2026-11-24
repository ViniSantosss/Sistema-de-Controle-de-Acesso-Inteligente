#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <cstdio>
#include <unistd.h>

#include "storage/userStorage.h"
#include "models/user.h"

extern Usuario usuarios[];
extern int quantidadeUsuarios;
extern int proximoId;

static constexpr char ARQUIVO_USUARIOS[] = "/usuarios.json";
static constexpr char ARQUIVO_TEMPORARIO[] = "/usuarios.tmp";
static constexpr char PONTO_MONTAGEM[] = "/littlefs";
static bool armazenamentoDisponivel = false;

void inicializarArmazenamentoUsuarios() {

    armazenamentoDisponivel = LittleFS.begin(false, PONTO_MONTAGEM);
    if (!armazenamentoDisponivel) {
        Serial.println("Erro ao iniciar o armazenamento de usuarios.");
        return;
    }

    Serial.println("Armazenamento de usuarios iniciado.");

    carregarUsuarios();
}

bool salvarUsuarios() {
    if (!armazenamentoDisponivel) {
        return false;
    }

    JsonDocument documento;
    JsonArray lista = documento.to<JsonArray>();

    for (int i = 0; i < quantidadeUsuarios; i++) {

        JsonObject usuario = lista.add<JsonObject>();

        usuario["id"] = usuarios[i].id;
        usuario["nome"] = usuarios[i].nome;
        usuario["tipoAcesso"] = usuarios[i].tipoAcesso;
        usuario["ativo"] = usuarios[i].ativo;
        usuario["credencialNFC"] = usuarios[i].credencialNFC;
        usuario["dataExpiracao"] = usuarios[i].dataExpiracao;
    }

    if (documento.overflowed()) {
        Serial.println("Memoria insuficiente para salvar usuarios.");
        return false;
    }

    const String caminhoTemporario = String(PONTO_MONTAGEM) + ARQUIVO_TEMPORARIO;
    FILE* arquivo = fopen(caminhoTemporario.c_str(), "wb");
    if (arquivo == nullptr) {
        Serial.println("Erro ao abrir arquivo temporario de usuarios.");
        return false;
    }

    struct GravadorArquivo {
        FILE* arquivo;

        size_t write(uint8_t byte) {
            return fputc(byte, arquivo) == EOF ? 0 : 1;
        }

        size_t write(const uint8_t* dados, size_t tamanho) {
            return fwrite(dados, 1, tamanho, arquivo);
        }
    } gravador{arquivo};

    const bool gravacaoCompleta =
        serializeJson(documento, gravador) == measureJson(documento) &&
        fflush(arquivo) == 0 && fsync(fileno(arquivo)) == 0;
    const bool arquivoFechado = fclose(arquivo) == 0;
    if (!gravacaoCompleta || !arquivoFechado ||
        !LittleFS.rename(ARQUIVO_TEMPORARIO, ARQUIVO_USUARIOS)) {
        LittleFS.remove(ARQUIVO_TEMPORARIO);
        Serial.println("Erro ao gravar todos os usuarios em usuarios.json.");
        return false;
    }

    Serial.println("Usuarios salvos.");
    return true;
}

void carregarUsuarios() {

    if (!LittleFS.exists(ARQUIVO_USUARIOS)) {
        Serial.println("Nenhum arquivo de usuarios encontrado.");
        return;
    }

    File arquivo = LittleFS.open(ARQUIVO_USUARIOS, "r");

    if (!arquivo) {
        Serial.println("Erro ao abrir usuarios.json.");
        return;
    }

    JsonDocument documento;

    DeserializationError erro = deserializeJson(documento, arquivo);

    arquivo.close();

    if (erro) {
        Serial.println("Erro ao ler usuarios.json.");
        return;
    }

    if (!documento.is<JsonArray>()) {
        Serial.println("Formato invalido em usuarios.json.");
        return;
    }

    quantidadeUsuarios = 0;
    proximoId = 1;

    for (JsonObject usuario : documento.as<JsonArray>()) {
        if (quantidadeUsuarios >= MAX_USUARIOS) {
            Serial.println("Limite de usuarios excedido em usuarios.json.");
            break;
        }

        usuarios[quantidadeUsuarios].id =
            usuario["id"] | 0;

        usuarios[quantidadeUsuarios].nome =
            usuario["nome"] | "";

        usuarios[quantidadeUsuarios].tipoAcesso =
            usuario["tipoAcesso"] | "";

        usuarios[quantidadeUsuarios].ativo =
            usuario["ativo"] | false;

        usuarios[quantidadeUsuarios].credencialNFC =
            usuario["credencialNFC"] | "";

        usuarios[quantidadeUsuarios].dataExpiracao =
            usuario["dataExpiracao"] | "";

        if (usuarios[quantidadeUsuarios].id >= proximoId) {
            proximoId = usuarios[quantidadeUsuarios].id + 1;
        }

        quantidadeUsuarios++;
    }

    Serial.print("Usuarios carregados: ");
    Serial.println(quantidadeUsuarios);
}
