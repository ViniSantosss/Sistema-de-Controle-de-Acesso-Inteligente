#include "nfc/nfcManager.h"

#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define PN532_IRQ 2
#define PN532_RESET 3

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

void inicializarNFC() {

    Wire.begin(SDA_PIN, SCL_PIN);

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();

    if (!versiondata) {
        Serial.println("PN532 nao encontrado.");
        return;
    }

    Serial.println("PN532 encontrado.");

    nfc.SAMConfig();

    Serial.println("Leitor NFC pronto.");
}

bool lerUID(String& uid) {

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
}