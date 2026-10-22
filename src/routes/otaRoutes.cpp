#include "routes/otaRoutes.h"

#include <Arduino.h>
#include <Update.h>

#include "routes/authRoutes.h"

static bool uploadAutorizado = false;
static bool uploadIniciado = false;
static bool uploadConcluido = false;
static bool uploadFalhou = false;
static bool primeiroByteValidado = false;
static size_t bytesRecebidos = 0;
static uint8_t proximoProgressoLog = 10;
static bool reinicioOtaPendente = false;
static uint32_t otaConcluidaEm = 0;

static bool nomeFirmwareValido(const String& nome) {
    String nomeMinusculo = nome;
    nomeMinusculo.toLowerCase();
    return nomeMinusculo.endsWith(".bin");
}

static void marcarFalhaOta(const char* contexto) {
    uploadFalhou = true;
    Serial.printf("OTA: erro em %s", contexto);

    if (Update.hasError()) {
        Serial.printf(": %s", Update.errorString());
    }

    Serial.println();
    if (Update.isRunning()) {
        Update.abort();
    }
}

static void receberFirmwareOta(WebServer& webServer) {
    HTTPUpload& upload = webServer.upload();

    if (upload.status == UPLOAD_FILE_START) {
        uploadAutorizado = administradorAutenticado(webServer);
        uploadIniciado = false;
        uploadConcluido = false;
        uploadFalhou = false;
        primeiroByteValidado = false;
        bytesRecebidos = 0;
        proximoProgressoLog = 10;

        if (!uploadAutorizado) {
            uploadFalhou = true;
            Serial.println("OTA: upload recusado; sessao administrativa ausente.");
            return;
        }

        Serial.printf("OTA: inicio do recebimento de '%s'.\n", upload.filename.c_str());
        if (!nomeFirmwareValido(upload.filename)) {
            uploadFalhou = true;
            Serial.println("OTA: arquivo recusado; esperado um arquivo .bin.");
            return;
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            marcarFalhaOta("inicializacao da particao OTA");
            return;
        }

        uploadIniciado = true;
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (!uploadAutorizado || !uploadIniciado || uploadFalhou) {
            return;
        }

        if (upload.currentSize == 0) {
            return;
        }

        if (!primeiroByteValidado) {
            if (upload.buf[0] != 0xE9) {
                marcarFalhaOta("validacao do formato do firmware");
                return;
            }
            primeiroByteValidado = true;
        }

        const size_t gravados = Update.write(upload.buf, upload.currentSize);
        if (gravados != upload.currentSize) {
            marcarFalhaOta("gravacao do firmware");
            return;
        }

        bytesRecebidos += gravados;
        if (upload.totalSize > 0) {
            const uint8_t progresso = static_cast<uint8_t>(
                (bytesRecebidos * 100) / upload.totalSize
            );
            if (progresso >= proximoProgressoLog) {
                Serial.printf(
                    "OTA: recebimento %u%%.\n",
                    static_cast<unsigned>(progresso)
                );
                proximoProgressoLog =
                    static_cast<uint8_t>(proximoProgressoLog + 10);
            }
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        if (!uploadAutorizado || !uploadIniciado || uploadFalhou) {
            return;
        }

        if (bytesRecebidos == 0 || !primeiroByteValidado) {
            marcarFalhaOta("validacao de arquivo vazio");
            return;
        }

        if (!Update.end(true)) {
            marcarFalhaOta("finalizacao da gravacao");
            return;
        }

        if (!Update.isFinished() || Update.hasError()) {
            marcarFalhaOta("validacao final do firmware");
            return;
        }

        uploadConcluido = true;
        uploadIniciado = false;
        Serial.printf(
            "OTA: firmware recebido e gravado (%u bytes).\n",
            static_cast<unsigned>(bytesRecebidos)
        );
        return;
    }

    if (upload.status == UPLOAD_FILE_ABORTED) {
        if (uploadIniciado) {
            Update.abort();
        }
        uploadIniciado = false;
        uploadFalhou = true;
        Serial.println("OTA: recebimento interrompido pelo cliente.");
    }
}

void configurarRotasOta(WebServer& webServer) {
    webServer.on(
        "/api/ota",
        HTTP_POST,
        [&webServer]() {
            const bool autorizado = administradorAutenticado(webServer);
            const bool arquivoRecebido = uploadAutorizado;
            const bool atualizacaoConcluida =
                uploadConcluido && !uploadFalhou;

            uploadAutorizado = false;
            uploadConcluido = false;
            uploadFalhou = false;

            if (!autorizado) {
                webServer.send(
                    401,
                    "application/json",
                    "{\"erro\":\"Sessao administrativa necessaria. Entre novamente.\"}"
                );
                return;
            }

            if (!arquivoRecebido) {
                webServer.send(
                    400,
                    "application/json",
                    "{\"erro\":\"Nenhum arquivo de firmware foi recebido.\"}"
                );
                return;
            }

            if (!atualizacaoConcluida) {
                webServer.send(
                    400,
                    "application/json",
                    "{\"erro\":\"Atualizacao nao concluida. Verifique o arquivo .bin e tente novamente.\"}"
                );
                return;
            }

            webServer.send(
                200,
                "application/json",
                "{\"mensagem\":\"Firmware gravado com sucesso. O ESP32 sera reiniciado.\"}"
            );
            webServer.client().flush();
            otaConcluidaEm = millis();
            reinicioOtaPendente = true;
        },
        [&webServer]() {
            receberFirmwareOta(webServer);
        }
    );
}

void processarReinicioOtaPendente() {
    if (!reinicioOtaPendente ||
        static_cast<uint32_t>(millis() - otaConcluidaEm) < 500) {
        return;
    }

    reinicioOtaPendente = false;
    Serial.println("OTA: atualizacao concluida; reiniciando ESP32.");
    ESP.restart();
}
