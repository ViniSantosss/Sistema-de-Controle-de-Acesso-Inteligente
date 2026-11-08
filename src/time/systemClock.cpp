#include "time/systemClock.h"

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

static constexpr char FUSO_HORARIO[] = "BRT3";
static constexpr char SERVIDOR_NTP_PRIMARIO[] = "pool.ntp.org";
static constexpr char SERVIDOR_NTP_SECUNDARIO[] = "time.google.com";
static constexpr time_t EPOCA_MINIMA_CONFIAVEL = 1704067200;

static volatile bool sincronizadoPorNtp = false;

static void aoSincronizarHorario(struct timeval* horario) {
    if (horario != nullptr && horario->tv_sec >= EPOCA_MINIMA_CONFIAVEL) {
        sincronizadoPorNtp = true;
    }
}

void iniciarRelogioSistema() {
    sincronizadoPorNtp = false;
    sntp_set_time_sync_notification_cb(aoSincronizarHorario);
    configTzTime(
        FUSO_HORARIO,
        SERVIDOR_NTP_PRIMARIO,
        SERVIDOR_NTP_SECUNDARIO
    );
}

bool horarioSistemaConfiavel() {
    return sincronizadoPorNtp &&
        time(nullptr) >= EPOCA_MINIMA_CONFIAVEL;
}

bool interpretarDataExpiracao(
    const String& dataHora,
    time_t& instante
) {
    if (dataHora.length() != 16) {
        return false;
    }

    const char* texto = dataHora.c_str();
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 7 || i == 13) {
            const char separador = i == 13 ? ':' : '-';
            if (texto[i] != separador) {
                return false;
            }
        } else if (i == 10) {
            if (texto[i] != 'T') {
                return false;
            }
        } else if (texto[i] < '0' || texto[i] > '9') {
            return false;
        }
    }

    int ano;
    int mes;
    int dia;
    int hora;
    int minuto;
    int caracteresLidos = 0;

    if (sscanf(
            dataHora.c_str(),
            "%4d-%2d-%2dT%2d:%2d%n",
            &ano,
            &mes,
            &dia,
            &hora,
            &minuto,
            &caracteresLidos
        ) != 5 ||
        caracteresLidos != 16 ||
        mes < 1 || mes > 12 ||
        dia < 1 || dia > 31 ||
        hora < 0 || hora > 23 ||
        minuto < 0 || minuto > 59) {
        return false;
    }

    struct tm data = {};
    data.tm_year = ano - 1900;
    data.tm_mon = mes - 1;
    data.tm_mday = dia;
    data.tm_hour = hora;
    data.tm_min = minuto;
    data.tm_isdst = -1;

    instante = mktime(&data);
    if (instante == static_cast<time_t>(-1)) {
        return false;
    }

    struct tm dataNormalizada = {};
    if (localtime_r(&instante, &dataNormalizada) == nullptr) {
        return false;
    }

    return dataNormalizada.tm_year == ano - 1900 &&
        dataNormalizada.tm_mon == mes - 1 &&
        dataNormalizada.tm_mday == dia &&
        dataNormalizada.tm_hour == hora &&
        dataNormalizada.tm_min == minuto;
}

String formatarDataExpiracao(const String& dataHora) {
    time_t instante;
    if (!interpretarDataExpiracao(dataHora, instante)) {
        return "";
    }

    struct tm data = {};
    if (localtime_r(&instante, &data) == nullptr) {
        return "";
    }

    char formatada[17];
    if (strftime(formatada, sizeof(formatada), "%d/%m/%Y %H:%M", &data) == 0) {
        return "";
    }

    return String(formatada);
}
