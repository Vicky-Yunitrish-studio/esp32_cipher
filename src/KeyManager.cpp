#include "KeyManager.h"

const char* KeyManager::KEY_FILE = "/encryption.key";

KeyManager::KeyManager() : initialized(false) {
    memset(key, 0, sizeof(key));
}

bool KeyManager::init() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    initialized = loadKey();
    if (!initialized) {
        generateRandomKey();
        initialized = saveKey(key);
    }
    return initialized;
}

bool KeyManager::loadKey() {
    if (!SPIFFS.exists(KEY_FILE)) {
        return false;
    }

    File file = SPIFFS.open(KEY_FILE, "r");
    if (!file) {
        return false;
    }

    if (file.size() != sizeof(key)) {
        file.close();
        return false;
    }

    size_t bytesRead = file.readBytes((char*)key, sizeof(key));
    file.close();
    return bytesRead == sizeof(key);
}

bool KeyManager::saveKey(const uint8_t* newKey) {
    File file = SPIFFS.open(KEY_FILE, "w");
    if (!file) {
        return false;
    }

    memcpy(key, newKey, sizeof(key));
    size_t bytesWritten = file.write(newKey, sizeof(key));
    file.close();
    return bytesWritten == sizeof(key);
}

void KeyManager::generateRandomKey() {
    for (int i = 0; i < 32; i++) {
        key[i] = random(256);
    }
}
