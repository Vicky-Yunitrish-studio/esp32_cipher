#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <Arduino.h>
#include "esp_system.h"
#include "mbedtls/aes.h"
#include "StorageManager.h"

class Encryption {
private:
    static const size_t KEY_SIZE = 32;
    static const size_t NONCE_SIZE = 16;
    
    uint8_t key[KEY_SIZE];
    uint8_t nonce[NONCE_SIZE];
    
    void generateNonce(const String& groupId, const String& macAddress);
    void deriveKey(const String& constant);
    size_t padData(uint8_t* data, size_t dataLen);

public:
    Encryption();
    void init(const String& groupId, const String& macAddress);
    String encrypt(const String& data);
    String decrypt(const String& encryptedData);
    void incrementNonce();
};

#endif
