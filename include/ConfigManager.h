#pragma once
#include <Arduino.h>
#include <SPIFFS.h>

class ConfigManager {
private:
    static const char* CONFIG_FILE;
    static const char* WIFI_FILE;
    char groupName[32];
    char wifiSSID[33];
    char wifiPassword[65];
    bool initialized;

    bool loadConfig();
    bool loadWiFiConfig();
    
public:
    ConfigManager();
    bool init();
    bool saveConfig(const char* group);
    bool saveWiFiConfig(const char* ssid, const char* password);
    const char* getGroupName() const { return groupName; }
    const char* getWiFiSSID() const { return wifiSSID; }
    const char* getWiFiPassword() const { return wifiPassword; }
    bool isInitialized() const { return initialized; }
};
