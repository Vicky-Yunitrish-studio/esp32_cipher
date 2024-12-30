#pragma once
#include <Arduino.h>
#include "Crypto.h"
#include "ChaCha.h"
#include "Poly1305.h"

class Encryption {
private:
    ChaCha chacha;
    Poly1305 poly;
    uint8_t key[32];
    uint8_t nonce[12];
    
public:
    Encryption();
    void init(const uint8_t* initKey);
    String encrypt(const String& plaintext);
    String decrypt(const String& ciphertext);
    void updateNonce();
};
