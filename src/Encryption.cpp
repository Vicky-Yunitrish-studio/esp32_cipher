#include "Encryption.h"
#include <string.h>

Encryption::Encryption() {
    memset(key, 0, sizeof(key));
    memset(nonce, 0, sizeof(nonce));
}

void Encryption::init(const uint8_t* initKey) {
    if (initKey != nullptr) {
        memcpy(key, initKey, 32);
        randomSeed(millis());
        for (int i = 0; i < 12; i++) {
            nonce[i] = random(256);
        }
    }
}

String Encryption::encrypt(const String& plaintext) {
    if (plaintext.length() == 0) {
        return "";
    }

    size_t msgLen = plaintext.length();
    uint8_t* ciphertext = new uint8_t[msgLen];
    uint8_t tag[16];
    
    // 初始化ChaCha20
    chacha.clear();
    chacha.setKey(key, 32);
    chacha.setIV(nonce, 12);
    
    // 加密
    chacha.encrypt((uint8_t*)ciphertext, (uint8_t*)plaintext.c_str(), msgLen);
    
    // 計算認證標籤
    poly.clear();
    poly.update(key, 32);
    poly.update(ciphertext, msgLen);
    poly.finalize(tag, nonce, 16);
    
    // 組合密文和標籤到十六進制字串
    String result;
    result.reserve((msgLen + 16) * 2);  // 預分配空間

    for(size_t i = 0; i < msgLen; i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", ciphertext[i]);
        result += hex;
    }
    
    for(size_t i = 0; i < 16; i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", tag[i]);
        result += hex;
    }
    
    delete[] ciphertext;
    updateNonce();
    return result;
}

void Encryption::updateNonce() {
    for (int i = 0; i < 12; i++) {
        if (++nonce[i] != 0) break;
    }
}
