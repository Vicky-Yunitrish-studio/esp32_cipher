#pragma once
#include <Arduino.h>
#include <SPIFFS.h>

class KeyManager {
private:
    static const char* KEY_FILE;
    uint8_t key[32];
    bool initialized;

public:
    KeyManager();
    bool init();
    bool loadKey();
    bool saveKey(const uint8_t* newKey);
    const uint8_t* getKey() const { return key; }
    bool isInitialized() const { return initialized; }
    void generateRandomKey();
};
