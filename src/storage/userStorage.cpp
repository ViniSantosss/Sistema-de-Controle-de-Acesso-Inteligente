#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "storage/userStorage.h"
#include "models/user.h"

extern Usuario usuarios[];
extern int quantidadeUsuarios;
extern int proximoId;

void inicializarArmazenamentoUsuarios() {

    if (!LittleFS.begin(true)) {
        Serial.println("Erro ao iniciar o armazenamento de usuarios.");
        return;
    }

    Serial.println("Armazenamento de usuarios iniciado.");

    carregarUsuarios();
}

bool salvarUsuarios() {

    File arquivo = LittleFS.open("/usuarios.json", "w");

    if (!arquivo) {
        Serial.println("Erro ao abrir usuarios.json para escrita.");
        return false;
    }

    JsonDocument documento;

    for (int i = 0; i < quantidadeUsuarios; i++) {

        JsonObject usuario = documento.add<JsonObject>();

        usuario["id"] = usuarios[i].id;
        usuario["nome"] = usuarios[i].nome;
        usuario["tipoAcesso"] = usuarios[i].tipoAcesso;
        usuario["ativo"] = usuarios[i].ativo;
        usuario["credencialNFC"] = usuarios[i].credencialNFC;
        usuario["dataExpiracao"] = usuarios[i].dataExpiracao;
    }

    size_t bytesEsperados = measureJson(documento);
    size_t bytesGravados = serializeJson(documento, arquivo);

    arquivo.close();

    if (bytesGravados != bytesEsperados) {
        Serial.println("Erro ao gravar todos os usuarios em usuarios.json.");
        return false;
    }

    Serial.println("Usuarios salvos.");
    return true;
}

void carregarUsuarios() {

    if (!LittleFS.exists("/usuarios.json")) {
        Serial.println("Nenhum arquivo de usuarios encontrado.");
        return;
    }

    File arquivo = LittleFS.open("/usuarios.json", "r");

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