#include "Encryption.h"

Encryption::Encryption() {}

void Encryption::init(const uint8_t* initKey) {
    memcpy(key, initKey, 32);
    // 生成初始nonce
    for (int i = 0; i < 12; i++) {
        nonce[i] = random(256);
    }
}

String Encryption::encrypt(const String& plaintext) {
    size_t msgLen = plaintext.length();
    uint8_t ciphertext[msgLen];
    uint8_t tag[16];
    
    // 初始化ChaCha20
    chacha.setKey(key, 32);
    chacha.setIV(nonce, 12);
    
    // 加密
    chacha.encrypt(ciphertext, (uint8_t*)plaintext.c_str(), msgLen);
    
    // 計算認證標籤
    poly.reset();
    poly.update(ciphertext, msgLen);
    poly.finalize(tag);
    
    // 組合密文和標籤
    String result;
    for(size_t i = 0; i < msgLen; i++) {
        char hex[3];
        sprintf(hex, "%02x", ciphertext[i]);
        result += hex;
    }
    for(size_t i = 0; i < 16; i++) {
        char hex[3];
        sprintf(hex, "%02x", tag[i]);
        result += hex;
    }
    
    updateNonce();
    return result;
}

void Encryption::updateNonce() {
    for (int i = 0; i < 12; i++) {
        if (++nonce[i] != 0) break;
    }
}
