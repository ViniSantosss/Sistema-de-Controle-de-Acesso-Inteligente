#include "network/wifiManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "routes/authRoutes.h"
#include "secrets.h"

static String rede, senha;
static uint32_t desconectadoDesde;
static bool apAtivo = false;
static bool testeAtivo = false;
static String novaRede, novaSenha;
static uint32_t inicioTeste;

static bool ativarAP() {
    if (apAtivo) return true;
    String chave = ESP32_PASSWORD;
    if (chave.length() < 8 || chave.length() > 63) {
        Serial.println("AP indisponivel: senha administrativa precisa ter 8 a 63 caracteres.");
        return false;
    }
    WiFi.mode(WIFI_AP_STA);
    apAtivo = WiFi.softAP("Controle-Acesso-Config", chave.c_str());
    if (apAtivo) Serial.println("AP de configuracao: 192.168.4.1");
    return apAtivo;
}

void iniciarWiFi() {
    rede = WIFI_SSID;
    senha = WIFI_PASSWORD;
    Preferences p;
    if (p.begin("wifi-config", true)) {
        String dados = p.getString("credenciais", "");
        JsonDocument doc;
        if (!deserializeJson(doc, dados) && doc["ssid"].is<const char*>() &&
            doc["senha"].is<const char*>()) {
            rede = doc["ssid"].as<String>();
            senha = doc["senha"].as<String>();
        }
        p.end();
    }
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(rede.c_str(), senha.c_str());
    desconectadoDesde = millis();
}

void atualizarModoAP() {
    if (testeAtivo) {
        if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == novaRede) {
            JsonDocument doc;
            doc["ssid"] = novaRede;
            doc["senha"] = novaSenha;
            String dados;
            serializeJson(doc, dados);
            Preferences p;
            bool salvo = false;
            if (p.begin("wifi-config", false)) {
                salvo = p.putString("credenciais", dados) == dados.length();
                p.end();
            }
            testeAtivo = false;
            if (salvo) {
                rede = novaRede;
                senha = novaSenha;
                Serial.print("Nova rede salva. IP: ");
                Serial.println(WiFi.localIP());
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                apAtivo = false;
            } else {
                WiFi.begin(rede.c_str(), senha.c_str());
            }
        } else if (millis() - inicioTeste >= 30000) {
            testeAtivo = false;
            WiFi.begin(rede.c_str(), senha.c_str());
            Serial.println("Falha na nova rede; restaurando rede anterior.");
        }
        if (!testeAtivo) { novaSenha = ""; WiFi.setAutoReconnect(true); }
        return;
    }
    if (WiFi.status() == WL_CONNECTED) desconectadoDesde = millis();
    else if (!apAtivo && millis() - desconectadoDesde >= 30000) {
        ativarAP();
        desconectadoDesde = millis();
    }
}

void configurarRotasWiFi(WebServer& server) {
    server.on("/api/wifi/ap", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        if (!ativarAP()) {
            server.send(500, "application/json", "{\"erro\":\"Falha ao ativar AP; verifique a senha administrativa.\"}");
            return;
        }
        server.send(200, "application/json", "{\"mensagem\":\"AP ativado.\"}");
    });
    server.on("/api/wifi/config", HTTP_POST, [&server]() {
        if (!exigirSessaoAdministrativa(server)) return;
        if (!apAtivo || testeAtivo) {
            server.send(409, "application/json", "{\"erro\":\"Ative o AP e aguarde o teste anterior.\"}");
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, server.arg("plain")) ||
            !doc["ssid"].is<const char*>() || !doc["senha"].is<const char*>()) {
            server.send(400, "application/json", "{\"erro\":\"Dados invalidos.\"}");
            return;
        }
        String s = doc["ssid"].as<String>(), p = doc["senha"].as<String>();
        if (!s.length() || s.length() > 32 || p.length() < 8 || p.length() > 63) {
            server.send(400, "application/json", "{\"erro\":\"SSID ou senha invalidos.\"}");
            return;
        }
        novaRede = s;
        novaSenha = p;
        server.send(202, "application/json", "{\"mensagem\":\"Testando nova rede. Aguarde ate 30 segundos.\"}");
        WiFi.setAutoReconnect(false);
        WiFi.disconnect(false, false);
        WiFi.begin(novaRede.c_str(), novaSenha.c_str());
        inicioTeste = millis();
        testeAtivo = true;
    });
}
