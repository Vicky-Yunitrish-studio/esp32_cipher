#pragma once
#include <Arduino.h>

class Encryption {
private:
    // ChaCha20 state
    uint32_t state[16];
    uint8_t key[32];
    uint8_t nonce[12];
    uint32_t counter;
    
    // Customizable constants
    uint32_t constants[4];

    // Poly1305 state
    uint32_t r[5];
    uint32_t h[5];
    uint32_t pad[4];
    uint8_t buffer[16];
    size_t buffer_size;

    // Internal methods
    void chacha20_block(uint32_t* output);
    void chacha20_encrypt(uint8_t* dst, const uint8_t* src, size_t len);
    void poly1305_init();
    void poly1305_update(const uint8_t* data, size_t len);
    void poly1305_finish(uint8_t* mac);
    
    // Helper functions
    uint32_t rotl32(uint32_t x, int n);
    void quarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d);

public:
    Encryption();
    void init(const uint8_t* initKey);
    void setConstants(const char* topic);  // New method
    String encrypt(const String& plaintext);
    String decrypt(const String& ciphertext);
    void updateNonce();
};
