#ifndef NFC_MANAGER_H
#define NFC_MANAGER_H

#include <Arduino.h>

void inicializarNFC();
bool lerUID(String& uid);

#endif