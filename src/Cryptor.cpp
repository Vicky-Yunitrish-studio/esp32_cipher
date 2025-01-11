#include "Cryptor.h"
#include <algorithm>
using std::min;

Cryptor::Cryptor() {
    memset(key, 0, KEY_SIZE);
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

void Cryptor::init(const String& macAddress, const uint8_t* customKey, size_t keyLength) {
    counter = 0;
    String formattedMac = formatMacAddress(macAddress);
    
    // Use MAC address as nonce
    for(size_t i = 0; i < min(NONCE_SIZE, formattedMac.length()); i++) {
        nonce[i] = formattedMac[i];
    }
    
    // Use custom key
    size_t keySize = min(KEY_SIZE, keyLength);
    memcpy(key, customKey, keySize);
    
    // If the provided key is shorter than KEY_SIZE, pad with MAC address
    if (keySize < KEY_SIZE) {
        for(size_t i = keySize; i < KEY_SIZE; i++) {
            key[i] = formattedMac[i % formattedMac.length()];
        }
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
    uint8_t* input = (uint8_t*)data.c_str();
    uint8_t* output = new uint8_t[dataLen];
    uint8_t keyStream[64];
    
    // Reset counter for each encryption
    uint32_t localCounter = counter;
    
    // Generate keystream and encrypt
    for(size_t i = 0; i < dataLen; i += 64) {
        chacha20Block(keyStream, key, localCounter++, nonce);
        size_t blockSize = min<size_t>(64UL, dataLen - i);
        for(size_t j = 0; j < blockSize; j++) {
            output[i + j] = input[i + j] ^ keyStream[j];
        }
    }
    
    // Update global counter
    counter = localCounter;
    
    // Convert to hex string
    String result;
    result.reserve(dataLen * 2); // Pre-allocate space
    for(size_t i = 0; i < dataLen; i++) {
        char hex[3];
        sprintf(hex, "%02x", output[i]);
        result += hex;
    }
    
    delete[] output;
    return result;
}

String Cryptor::decrypt(const String& encryptedData) {
    size_t dataLen = encryptedData.length() / 2;
    uint8_t* input = new uint8_t[dataLen];
    uint8_t* output = new uint8_t[dataLen + 1]; // +1 for null terminator
    uint8_t keyStream[64];
    
    // Convert hex string to bytes
    for(size_t i = 0; i < dataLen; i++) {
        String byteStr = encryptedData.substring(i*2, i*2+2);
        input[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    
    // Reset counter for each decryption
    uint32_t localCounter = counter;
    
    // Decrypt using keystream
    for(size_t i = 0; i < dataLen; i += 64) {
        chacha20Block(keyStream, key, localCounter++, nonce);
        size_t blockSize = min<size_t>(64UL, dataLen - i);
        for(size_t j = 0; j < blockSize; j++) {
            output[i + j] = input[i + j] ^ keyStream[j];
        }
    }
    
    // Update global counter
    counter = localCounter;
    
    // Ensure null termination
    output[dataLen] = 0;
    String result((char*)output);
    
    delete[] input;
    delete[] output;
    return result;
}