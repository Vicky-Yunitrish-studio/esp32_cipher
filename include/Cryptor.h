#pragma once
#include <Arduino.h>

class Cryptor {
    private:
        static const size_t KEY_SIZE = 32;
        static const size_t NONCE_SIZE = 12;
        static const size_t TAG_SIZE = 16;
        
        uint8_t key[KEY_SIZE];
        uint8_t nonce[NONCE_SIZE];
        uint32_t counter;
        
        String formatMacAddress(const String& macAddress);
        void chacha20Block(uint8_t output[64], const uint8_t key[32], uint32_t counter, const uint8_t nonce[12]);
        void poly1305Init(uint8_t r[16], uint8_t s[16], const uint8_t key[32]);
        void poly1305Update(uint8_t acc[16], const uint8_t r[16], const uint8_t* data, size_t len);
        void poly1305Finish(uint8_t tag[16], const uint8_t acc[16], const uint8_t s[16]);
        
    public:
        Cryptor();
        void init(const String& macAddress);
        String encrypt(const String& data);
        String decrypt(const String& encryptedData);
        void resetCounter() { counter = 0; }  // Add this method
};