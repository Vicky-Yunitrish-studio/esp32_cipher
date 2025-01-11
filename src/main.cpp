#include <Arduino.h>
#include "Cryptor.h"
#include "StorageManager.h"
#include "Connect.h"
#include <WiFi.h>

Cryptor cryptor;
StorageManager storage;
Connect connect;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // 等待Serial連接
    }
    
    Serial.println("Starting ESP32...");
    
    // 初始化SPIFFS和StorageManager
    if (!storage.init()) {
        Serial.println("Storage initialization failed!");
        return;
    }
    
    // 確保WiFi設定被正確保存
    if (!storage.saveWiFiConfig("Yun", "0937565253")) {
        Serial.println("Failed to save WiFi config!");
        return;
    }
    
    if (!storage.saveConfig("test_group")) {
        Serial.println("Failed to save group config!");
        return;
    }
    
    // 驗證設定是否正確保存
    Serial.println("Stored WiFi SSID: " + String(storage.getWiFiSSID()));
    Serial.println("Stored Group: " + String(storage.getGroupName()));
    
    // 設定WiFi參數
    connect.ssid = storage.getWiFiSSID();
    connect.status = "Initializing...";
    Serial.println("Connecting to WiFi: " + connect.ssid);
    
    // 開始WiFi連線
    connect.setup();
    
    // 等待WiFi連線
    while (!connect.isConnected() && connect.numberOfTries > 0) {
        connect.link();
        Serial.println(connect.status);
        delay(500);
    }
    
    if (!connect.isConnected()) {
        Serial.println("WiFi connection failed!");
        return;
    }
    
    Serial.println("WiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    
    // 獲取MAC地址
    String macAddress = WiFi.macAddress();
    Serial.println("ESP32 MAC Address: " + macAddress);
    
    // 從storage獲取金鑰並初始化Cryptor
    const uint8_t* storedKey = storage.getKey();
    Serial.println("\nStored Key (HEX):");
    for(int i = 0; i < 32; i++) {
        if(storedKey[i] < 0x10) Serial.print("0");
        Serial.print(storedKey[i], HEX);
    }
    Serial.println();
    
    // 使用儲存的金鑰初始化Cryptor
    cryptor.init(macAddress, storedKey, 32);
    
    // 測試加密解密
    const char* testMessages[] = {
        "Hello, World!",
        "This is a secret message",
        "Testing ESP32 encryption",
        "1234567890"
    };
    
    Serial.println("\nStarting encryption/decryption tests with stored key:");
    Serial.println("=================================================");
    
    for (const char* msg : testMessages) {
        cryptor.resetCounter();
        Serial.println("\nOriginal message: " + String(msg));
        
        // 加密
        String encrypted = cryptor.encrypt(msg);
        Serial.println("Encrypted (HEX): " + encrypted);
        
        // Reset counter for decryption
        cryptor.resetCounter();
        
        // 解密
        String decrypted = cryptor.decrypt(encrypted);
        Serial.println("Decrypted: " + decrypted);
        
        // 驗證
        if (String(msg) == decrypted) {
            Serial.println("✓ Test passed!");
        } else {
            Serial.println("✗ Test failed!");
        }
    }
}

void loop() {
    // 檢查WiFi連線狀態
    if (!connect.isConnected()) {
        Serial.println("WiFi connection lost, attempting to reconnect...");
        connect.setup();
        delay(5000);  // 等待5秒後重試
    }
    
    delay(1000);
}
