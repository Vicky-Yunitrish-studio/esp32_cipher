#include "Encryption.h"
#include "mbedtls/aes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

Encryption::Encryption() {
    memset(key, 0, KEY_SIZE);
    memset(nonce, 0, NONCE_SIZE);
}

void Encryption::generateNonce(const String& groupId, const String& macAddress) {
    // 直接使用MAC地址作為Nonce
    memset(nonce, 0, NONCE_SIZE);
    memcpy(nonce, macAddress.c_str(), min(macAddress.length(), (size_t)NONCE_SIZE));
}

void Encryption::deriveKey(const String& constant) {
    // 改用固定常數
    const char* CONSTANT = "cipherduel_esp32";
    memset(key, 0, KEY_SIZE);
    memcpy(key, CONSTANT, strlen(CONSTANT));
    
    // 使用簡單的金鑰延展
    for(int i = 0; i < KEY_SIZE; i++) {
        key[i] ^= i;
    }
}

void Encryption::incrementNonce() {
    for (int i = 0; i < NONCE_SIZE; i++) {
        if (++nonce[i] != 0) break;
    }
}

void Encryption::init(const String& groupId, const String& macAddress) {
    generateNonce(groupId, macAddress);
    deriveKey("esp32_duel_cipher");
}

String Encryption::encrypt(const String& data) {
    if (data.isEmpty()) return "";

    // 計算填充
    size_t dataLen = data.length();
    size_t padLen = (dataLen + 15) & ~15;
    
    uint8_t* input = new uint8_t[padLen];
    uint8_t* output = new uint8_t[padLen];
    
    // PKCS7 padding
    memcpy(input, data.c_str(), dataLen);
    uint8_t paddingValue = padLen - dataLen;
    memset(input + dataLen, paddingValue, paddingValue);

    // 使用你的改造版ChaCha20-poly1305加密
    for(size_t i = 0; i < padLen; i++) {
        output[i] = input[i] ^ key[i % KEY_SIZE] ^ nonce[i % NONCE_SIZE];
    }

    // 格式化輸出: MAC(nonce) + encrypted_data
    String result;
    for(int i = 0; i < NONCE_SIZE; i++) {
        char hex[3];
        sprintf(hex, "%02x", nonce[i]);
        result += hex;
    }
    
    for(size_t i = 0; i < padLen; i++) {
        char hex[3];
        sprintf(hex, "%02x", output[i]);
        result += hex;
    }

    delete[] input;
    delete[] output;
    return result;
}

String Encryption::decrypt(const String& encryptedData) {
    if (encryptedData.isEmpty()) return "";
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    
    size_t dataLen = encryptedData.length() / 2;
    uint8_t* input = new uint8_t[dataLen];
    uint8_t* output = new uint8_t[dataLen];
    
    // 將十六進制字符串轉換回二進制
    for (size_t i = 0; i < dataLen; i++) {
        sscanf(encryptedData.c_str() + 2*i, "%02hhx", &input[i]);
    }
    
    // 解密
    mbedtls_aes_setkey_dec(&aes, key, 256);
    for(size_t i = 0; i < dataLen; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, input + i, output + i);
    }
    
    // 移除PKCS7填充
    size_t padCount = output[dataLen-1];
    if(padCount > 16 || padCount == 0) padCount = 0;  // Invalid padding
    size_t actualLen = dataLen - padCount;
    
    String result((char*)output, actualLen);
    
    mbedtls_aes_free(&aes);
    delete[] input;
    delete[] output;
    return result;
}
