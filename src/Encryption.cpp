#include "Encryption.h"
#include <string.h>

Encryption::Encryption() : counter(1), buffer_size(0) {
    memset(key, 0, sizeof(key));
    memset(nonce, 0, sizeof(nonce));
}

uint32_t Encryption::rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

void Encryption::quarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
    a += b; d ^= a; d = rotl32(d, 16);
    c += d; b ^= c; b = rotl32(b, 12);
    a += b; d ^= a; d = rotl32(d, 8);
    c += d; b ^= c; b = rotl32(b, 7);
}

void Encryption::setConstants(const char* topic) {
    size_t len = strlen(topic);
    uint32_t hash[4] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574}; // Default values
    
    // Create unique constants based on topic
    for(size_t i = 0; i < len; i++) {
        hash[i % 4] ^= (topic[i] << ((i % 4) * 8));
    }
    
    memcpy(constants, hash, sizeof(constants));
}

void Encryption::chacha20_block(uint32_t* output) {
    // Use custom constants instead of hardcoded ones
    memcpy(state, constants, 16);
    memcpy(state + 4, key, 32);
    state[12] = counter;
    memcpy(state + 13, nonce, 12);

    // 20 rounds (10 iterations of double round)
    for (int i = 0; i < 10; i++) {
        // Column rounds
        quarterRound(state[0], state[4], state[8], state[12]);
        quarterRound(state[1], state[5], state[9], state[13]);
        quarterRound(state[2], state[6], state[10], state[14]);
        quarterRound(state[3], state[7], state[11], state[15]);
        
        // Diagonal rounds
        quarterRound(state[0], state[5], state[10], state[15]);
        quarterRound(state[1], state[6], state[11], state[12]);
        quarterRound(state[2], state[7], state[8], state[13]);
        quarterRound(state[3], state[4], state[9], state[14]);
    }

    // Copy output
    memcpy(output, state, 64);
}

void Encryption::chacha20_encrypt(uint8_t* dst, const uint8_t* src, size_t len) {
    uint32_t block[16];
    uint8_t* keystream = (uint8_t*)block;
    
    for (size_t i = 0; i < len; i += 64) {
        chacha20_block(block);
        counter++;
        
        size_t chunk_len = std::min(size_t(64), len - i);
        for (size_t j = 0; j < chunk_len; j++) {
            dst[i + j] = src[i + j] ^ keystream[j];
        }
    }
}

String Encryption::encrypt(const String& plaintext) {
    size_t len = plaintext.length();
    uint8_t* output = new uint8_t[len];
    
    chacha20_encrypt(output, (uint8_t*)plaintext.c_str(), len);
    
    // Convert to base64 or hex string for transmission
    String result;
    for (size_t i = 0; i < len; i++) {
        char hex[3];
        sprintf(hex, "%02x", output[i]);
        result += hex;
    }
    
    delete[] output;
    return result;
}

String Encryption::decrypt(const String& ciphertext) {
    // Convert hex string back to bytes
    size_t len = ciphertext.length() / 2;
    uint8_t* input = new uint8_t[len];
    uint8_t* output = new uint8_t[len];
    
    for (size_t i = 0; i < len; i++) {
        sscanf(ciphertext.c_str() + 2*i, "%2hhx", &input[i]);
    }
    
    chacha20_encrypt(output, input, len);  // ChaCha20 is symmetric
    
    String result((char*)output, len);
    
    delete[] input;
    delete[] output;
    return result;
}

void Encryption::init(const uint8_t* initKey) {
    memcpy(key, initKey, 32);
    updateNonce();
}

void Encryption::updateNonce() {
    // Generate random nonce
    for (int i = 0; i < 12; i++) {
        nonce[i] = random(256);
    }
    counter = 1;
}
