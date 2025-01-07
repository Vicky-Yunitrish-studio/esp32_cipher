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
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    
    // Initialize key
    initialized = loadKey();
    if (!initialized) {
        generateRandomKey();
        initialized = saveKey(key);
    }
    
    // Initialize config
    bool configLoaded = loadConfig() && loadWiFiConfig();
    if (!configLoaded) {
        // Set default values using parameters
        strcpy(groupName, defaultGroup);
        strcpy(wifiSSID, defaultSSID);
        strcpy(wifiPassword, defaultPassword);
        
        // Save default values
        saveConfig(groupName);
        saveWiFiConfig(wifiSSID, wifiPassword);
    }
    
    // Setup MQTT topics after initialization
    setupTopics();
    
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
    if (!file) return false;
    
    file.println(ssid);
    file.println(password);
    strncpy(wifiSSID, ssid, sizeof(wifiSSID) - 1);
    strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
    
    file.close();
    return true;
}

void StorageManager::setupTopics() {
    mqttBaseTopic = String("esp32/") + getGroupName() + "/" + deviceMac;
    mqttTempTopic = mqttBaseTopic + "/temp";
    mqttHumTopic = mqttBaseTopic + "/hum";
}
