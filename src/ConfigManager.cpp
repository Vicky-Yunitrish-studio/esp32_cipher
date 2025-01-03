#include "ConfigManager.h"

const char* ConfigManager::CONFIG_FILE = "/config.txt";
const char* ConfigManager::WIFI_FILE = "/wifi.txt";

ConfigManager::ConfigManager() : initialized(false) {
    memset(groupName, 0, sizeof(groupName));
    memset(wifiSSID, 0, sizeof(wifiSSID));
    memset(wifiPassword, 0, sizeof(wifiPassword));
}

bool ConfigManager::init() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    
    initialized = loadConfig() && loadWiFiConfig();
    if (!initialized) {
        // 設置預設值
        strcpy(groupName, "group1");
        strcpy(wifiSSID, "default_ssid");
        strcpy(wifiPassword, "default_password");
        
        // 保存預設值
        saveConfig(groupName);
        saveWiFiConfig(wifiSSID, wifiPassword);
        initialized = true;
    }
    return initialized;
}

bool ConfigManager::loadConfig() {
    if (!SPIFFS.exists(CONFIG_FILE)) return false;
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) return false;
    
    String group = file.readStringUntil('\n');
    group.trim();
    strncpy(groupName, group.c_str(), sizeof(groupName) - 1);
    
    file.close();
    return true;
}

bool ConfigManager::loadWiFiConfig() {
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

bool ConfigManager::saveConfig(const char* group) {
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) return false;
    
    file.println(group);
    strncpy(groupName, group, sizeof(groupName) - 1);
    
    file.close();
    return true;
}

bool ConfigManager::saveWiFiConfig(const char* ssid, const char* password) {
    File file = SPIFFS.open(WIFI_FILE, "w");
    if (!file) return false;
    
    file.println(ssid);
    file.println(password);
    strncpy(wifiSSID, ssid, sizeof(wifiSSID) - 1);
    strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
    
    file.close();
    return true;
}
