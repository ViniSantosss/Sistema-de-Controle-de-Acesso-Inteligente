#include "nfc/nfcManager.h"

#define NFC_HABILITADO 1

#if NFC_HABILITADO
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SCK 18
#define PN532_MISO 19
#define PN532_MOSI 23
#define PN532_SS 5

Adafruit_PN532 nfc(PN532_SS, &SPI);
bool leitorNFCPronto = false;
#endif

void inicializarNFC() {

#if NFC_HABILITADO
    leitorNFCPronto = false;
    SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

    if (!nfc.begin()) {
        Serial.println("Erro ao iniciar o PN532 no barramento SPI.");
        return;
    }

    uint32_t versiondata = nfc.getFirmwareVersion();

    if (!versiondata) {
        Serial.println("PN532 nao respondeu no barramento SPI.");
        return;
    }

    Serial.println("PN532 encontrado.");

    if (!nfc.SAMConfig()) {
        Serial.println("Falha ao configurar o leitor PN532.");
        return;
    }

    leitorNFCPronto = true;
    Serial.println("Leitor NFC pronto.");
#else
    Serial.println("Leitor NFC desativado temporariamente.");
#endif
}

bool lerUID(String& uid) {

#if NFC_HABILITADO
    if (!leitorNFCPronto) {
        return false;
    }

    uint8_t success;

    uint8_t uidBytes[7];
    uint8_t uidLength;

    success = nfc.readPassiveTargetID(
        PN532_MIFARE_ISO14443A,
        uidBytes,
        &uidLength,
        200
    );

    if (!success) {
        return false;
    }

    uid = "";

    for (uint8_t i = 0; i < uidLength; i++) {

        if (uidBytes[i] < 0x10) {
            uid += "0";
        }

        uid += String(uidBytes[i], HEX);

        if (i < uidLength - 1) {
            uid += ":";
        }
    }

    uid.toUpperCase();

    return true;
#else
    (void)uid;
    return false;
#endif
}