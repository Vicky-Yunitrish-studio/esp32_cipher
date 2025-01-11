#include "Cryptor.h"
#include <algorithm>
using std::min;

Cryptor::Cryptor() {
    memset(key, 0, KEY_SIZE);
    memset(nonce, 0, NONCE_SIZE);
    memset(nonce, 0, NONCE_SIZE);
}

String Cryptor::formatMacAddress(const String& macAddress) {
    String formatted = "";
    for(char c : macAddress) {
        if(c != ':') formatted += c;
    }
    return formatted;
}

void Cryptor::init(const String& macAddress) {
    counter = 0;
    String formattedMac = formatMacAddress(macAddress);
    
    // Use MAC address as nonce
    for(size_t i = 0; i < min(NONCE_SIZE, formattedMac.length()); i++) {
        nonce[i] = formattedMac[i];
    }
    
    // Generate key from MAC address (for demonstration - in production use a secure key)
    for(size_t i = 0; i < KEY_SIZE; i++) {
        key[i] = formattedMac[i % formattedMac.length()];
    }
}

void Cryptor::chacha20Block(uint8_t output[64], const uint8_t key[32], uint32_t counter, const uint8_t nonce[12]) {
    uint32_t state[16];
    // ChaCha20 constants
    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;
    
    // Key
    for (int i = 0; i < 8; i++) {
        state[4 + i] = ((uint32_t)key[4*i]) |
                       ((uint32_t)key[4*i + 1] << 8) |
                       ((uint32_t)key[4*i + 2] << 16) |
                       ((uint32_t)key[4*i + 3] << 24);
    }
    
    // Counter
    state[12] = counter;
    
    // Nonce
    for (int i = 0; i < 3; i++) {
        state[13 + i] = ((uint32_t)nonce[4*i]) |
                        ((uint32_t)nonce[4*i + 1] << 8) |
                        ((uint32_t)nonce[4*i + 2] << 16) |
                        ((uint32_t)nonce[4*i + 3] << 24);
    }
    
    uint32_t working[16];
    memcpy(working, state, 64);
    
    // 20 rounds (10 diagonal rounds)
    for (int i = 0; i < 10; i++) {
        // Column rounds
        working[0]  += working[4];  working[12] ^= working[0];  working[12] = (working[12] << 16) | (working[12] >> 16);
        working[8]  += working[12]; working[4]  ^= working[8];  working[4]  = (working[4]  << 12) | (working[4]  >> 20);
        working[0]  += working[4];  working[12] ^= working[0];  working[12] = (working[12] << 8)  | (working[12] >> 24);
        working[8]  += working[12]; working[4]  ^= working[8];  working[4]  = (working[4]  << 7)  | (working[4]  >> 25);
        
        // Diagonal rounds
        working[1]  += working[5];  working[13] ^= working[1];  working[13] = (working[13] << 16) | (working[13] >> 16);
        working[9]  += working[13]; working[5]  ^= working[9];  working[5]  = (working[5]  << 12) | (working[5]  >> 20);
        working[1]  += working[5];  working[13] ^= working[1];  working[13] = (working[13] << 8)  | (working[13] >> 24);
        working[9]  += working[13]; working[5]  ^= working[9];  working[5]  = (working[5]  << 7)  | (working[5]  >> 25);
    }
    
    for (int i = 0; i < 16; i++) {
        working[i] += state[i];
    }
    
    for (int i = 0; i < 16; i++) {
        output[4*i]     = working[i] & 0xff;
        output[4*i + 1] = (working[i] >> 8) & 0xff;
        output[4*i + 2] = (working[i] >> 16) & 0xff;
        output[4*i + 3] = (working[i] >> 24) & 0xff;
    }
}

String Cryptor::encrypt(const String& data) {
    size_t dataLen = data.length();
    uint8_t* output = new uint8_t[dataLen + TAG_SIZE];
    uint8_t keyStream[64];
    
    // Generate keystream and encrypt
    for(size_t i = 0; i < dataLen; i += 64) {
        chacha20Block(keyStream, key, counter++, nonce);
        for(size_t j = 0; j < min<size_t>(64UL, dataLen - i); j++) {
            output[i + j] = data[i + j] ^ keyStream[j];
        }
    }
    
    // Convert to hex string
    String result;
    for(size_t i = 0; i < dataLen; i++) {
        if(output[i] < 16) result += "0";
        result += String(output[i], HEX);
    }
    
    delete[] output;
    return result;
}

String Cryptor::decrypt(const String& encryptedData) {
    size_t dataLen = encryptedData.length() / 2;
    uint8_t* input = new uint8_t[dataLen];
    uint8_t* output = new uint8_t[dataLen];
    uint8_t keyStream[64];
    
    // Convert hex string to bytes
    for(size_t i = 0; i < dataLen; i++) {
        String byteStr = encryptedData.substring(i*2, i*2+2);
        input[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    
    // Decrypt using keystream
    for(size_t i = 0; i < dataLen; i += 64) {
        chacha20Block(keyStream, key, counter++, nonce);
        for(size_t j = 0; j < min<size_t>(64UL, dataLen - i); j++) {
            output[i + j] = input[i + j] ^ keyStream[j];
        }
    }
    
    String result = String((char*)output, dataLen);
    
    delete[] input;
    delete[] output;
    return result;
}