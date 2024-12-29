#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

// WiFi 設定
const char* ssid = "Yun";
const char* password = "0937565253";

// 重試次數設定
const int maxRetries = 20;
const int retryDelay = 500; // 毫秒

// 連線測試參數
const int pingInterval = 10000;  // 每10秒進行一次連線測試
const int testCount = 10;        // 進行10次測試

void printWiFiStatus() {
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.println(WiFi.RSSI());
}

bool connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
        delay(retryDelay);
        Serial.print(".");
        retries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected!");
        printWiFiStatus();
        return true;
    } else {
        Serial.println("WiFi Connection Failed!");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nWiFi Connection Test Starting...");
    
    if (!connectToWiFi()) {
        Serial.println("Initial connection failed, entering retry loop");
    }
}

void loop() {
    // 監控 WiFi 狀態
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost! Attempting to reconnect...");
        WiFi.disconnect();
        delay(1000);
        connectToWiFi();
    }

    // 每5秒顯示一次連線狀態
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        Serial.printf("WiFi Status: %s\n", 
            WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        if (WiFi.status() == WL_CONNECTED) {
            printWiFiStatus();
        }
    }
    
    static int testCounter = 0;
    static unsigned long lastTestTime = 0;
    
    if (millis() - lastTestTime >= pingInterval && testCounter < testCount) {
        lastTestTime = millis();
        testCounter++;
        
        // 執行連線測試
        Serial.printf("\n=== 測試 #%d ===\n", testCounter);
        Serial.printf("連線狀態: %s\n", 
            WiFi.status() == WL_CONNECTED ? "已連接" : "未連接");
        Serial.printf("訊號強度(RSSI): %d dBm\n", WiFi.RSSI());
        Serial.printf("IP位址: %s\n", WiFi.localIP().toString().c_str());
        
        // 計算連線品質
        int quality = 2 * (WiFi.RSSI() + 100);
        quality = quality > 100 ? 100 : quality;
        quality = quality < 0 ? 0 : quality;
        Serial.printf("連線品質: %d%%\n", quality);
    }
    
    if (testCounter >= testCount) {
        Serial.println("\n測試完成!");
        delay(30000);  // 等待30秒後重新開始測試
        testCounter = 0;
    }
    
    delay(100);
}
