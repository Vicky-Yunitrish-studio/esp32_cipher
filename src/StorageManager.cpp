#include "StorageManager.h"
#include <WiFi.h>

const char* StorageManager::KEY_FILE = "/encryption.key";
const char* StorageManager::CONFIG_FILE = "/config.txt";
const char* StorageManager::WIFI_FILE = "/wifi.txt";

StorageManager::StorageManager() : initialized(false) {
    memset(key, 0, sizeof(key));
    memset(groupName, 0, sizeof(groupName));
    memset(wifiSSID, 0, sizeof(wifiSSID));
    memset(wifiPassword, 0, sizeof(wifiPassword));
    deviceMac = WiFi.macAddress();
    deviceMac.replace(":", "");
}

bool StorageManager::init(const char* defaultGroup, const char* defaultSSID, const char* defaultPassword) {
    // 結束之前的SPIFFS會話（如果有的話）
    SPIFFS.end();
    
    // 初始化SPIFFS，true參數會格式化檔案系統如果掛載失敗
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    
    Serial.println("SPIFFS mounted successfully");
    
    // 初始化加密金鑰
    initialized = loadKey();
    if (!initialized) {
        Serial.println("Generating new encryption key...");
        generateRandomKey();
        initialized = saveKey(key);
    }
    
    // 嘗試載入現有配置
    bool configLoaded = loadConfig();
    bool wifiLoaded = loadWiFiConfig();
    
    // 如果配置不存在或無效，使用預設值
    if (!configLoaded && defaultGroup) {
        Serial.println("Saving default group config...");
        saveConfig(defaultGroup);
    }
    
    if (!wifiLoaded && defaultSSID && defaultPassword) {
        Serial.println("Saving default WiFi config...");
        saveWiFiConfig(defaultSSID, defaultPassword);
    }
    
    // 設置MQTT主題
    setupTopics();
    
    // 列出SPIFFS中的所有文件（用於偵錯）
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("SPIFFS files:");
    while(file) {
        Serial.print("- ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
    
    return initialized;
}

bool StorageManager::loadKey() {
    if (!SPIFFS.exists(KEY_FILE)) {
        return false;
    }

    File file = SPIFFS.open(KEY_FILE, "r");
    if (!file) {
        return false;
    }

    if (file.size() != sizeof(key)) {
        file.close();
        return false;
    }

    size_t bytesRead = file.readBytes((char*)key, sizeof(key));
    file.close();
    return bytesRead == sizeof(key);
}

bool StorageManager::saveKey(const uint8_t* newKey) {
    File file = SPIFFS.open(KEY_FILE, "w");
    if (!file) {
        return false;
    }

    memcpy(key, newKey, sizeof(key));
    size_t bytesWritten = file.write(newKey, sizeof(key));
    file.close();
    return bytesWritten == sizeof(key);
}

void StorageManager::generateRandomKey() {
    for (int i = 0; i < 32; i++) {
        key[i] = random(256);
    }
}

bool StorageManager::loadConfig() {
    if (!SPIFFS.exists(CONFIG_FILE)) return false;
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) return false;
    
    String group = file.readStringUntil('\n');
    group.trim();
    strncpy(groupName, group.c_str(), sizeof(groupName) - 1);
    
    file.close();
    return true;
}

bool StorageManager::loadWiFiConfig() {
    if (!SPIFFS.exists(WIFI_FILE)) return false;
    
    File file = SPIFFS.open(WIFI_FILE, "r");
    if (!file) return false;
    
    String ssid = file.readStringUntil('\n');
    String password = file.readStringUntil('\n');
    ssid.trim();
    password.trim();
    
    strncpy(wifiSSID, ssid.c_str(), sizeof(wifiSSID) - 1);
    strncpy(wifiPassword, password.c_str(), sizeof(wifiPassword) - 1);
    
    file.close();
    return true;
}

bool StorageManager::saveConfig(const char* group) {
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) return false;
    
    file.println(group);
    strncpy(groupName, group, sizeof(groupName) - 1);
    
    file.close();
    return true;
}

bool StorageManager::saveWiFiConfig(const char* ssid, const char* password) {
    File file = SPIFFS.open(WIFI_FILE, "w");
    if (!file) {
        Serial.println("Failed to open WiFi config file for writing");
        return false;
    }
    
    // 寫入並立即刷新到儲存設備
    file.println(ssid);
    file.println(password);
    file.flush();
    
    // 更新內部儲存的值
    strncpy(wifiSSID, ssid, sizeof(wifiSSID) - 1);
    strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
    
    file.close();
    
    // 驗證寫入是否成功
    if (loadWiFiConfig()) {
        Serial.println("WiFi config saved and verified");
        return true;
    }
    
    Serial.println("WiFi config save verification failed");
    return false;
}

void StorageManager::setupTopics() {
    mqttBaseTopic = String("esp32/") + getGroupName() + "/" + deviceMac;
    mqttTempTopic = mqttBaseTopic + "/temp";
    mqttHumTopic = mqttBaseTopic + "/hum";
}

String StorageManager::getMqttTempTopic() {
    return String("duel_cipher32/") + getGroupName() + "/" + getDeviceMac() + "/temperature";
}

String StorageManager::getMqttHumTopic() {
    return String("duel_cipher32/") + getGroupName() + "/" + getDeviceMac() + "/humidity";
}
