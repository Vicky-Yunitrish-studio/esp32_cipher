#include "Encryption.h"
#include "mbedtls/aes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

Encryption::Encryption() {
    memset(key, 0, KEY_SIZE);
    memset(nonce, 0, NONCE_SIZE);
}

void Encryption::generateNonce(const String& groupId, const String& macAddress) {
    String seed = groupId + macAddress;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                          (const unsigned char*)seed.c_str(), seed.length());
    
    mbedtls_ctr_drbg_random(&ctr_drbg, nonce, NONCE_SIZE);
    
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

void Encryption::deriveKey(const String& constant) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    
    // 使用constant作為初始key
    memset(key, 0, KEY_SIZE);
    memcpy(key, constant.c_str(), min(constant.length(), (size_t)KEY_SIZE));
    
    // 使用AES-256進行金鑰延展
    mbedtls_aes_setkey_enc(&aes, key, 256);
    uint8_t temp_key[KEY_SIZE] = {0};
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, key, temp_key);
    memcpy(key, temp_key, KEY_SIZE);
    
    mbedtls_aes_free(&aes);
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

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    
    size_t dataLen = data.length();
    size_t padLen = (dataLen + 15) & ~15;  // Round up to multiple of 16
    uint8_t* input = new uint8_t[padLen];
    uint8_t* output = new uint8_t[padLen];
    
    // 填充數據
    memset(input, padLen - dataLen, padLen);  // PKCS7 padding
    memcpy(input, data.c_str(), dataLen);
    
    // 加密
    mbedtls_aes_setkey_enc(&aes, key, 256);
    for(size_t i = 0; i < padLen; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input + i, output + i);
    }
    
    // 轉換為十六進制字符串
    String result;
    for (size_t i = 0; i < padLen; i++) {
        char hex[3];
        sprintf(hex, "%02x", output[i]);
        result += hex;
    }
    
    mbedtls_aes_free(&aes);
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
