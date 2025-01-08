#pragma once
#include <Arduino.h>
#include <SPIFFS.h>

class StorageManager {
private:
    static const char* KEY_FILE;
    static const char* CONFIG_FILE;
    static const char* WIFI_FILE;
    
    uint8_t key[32];
    char groupName[32];
    char wifiSSID[33];
    char wifiPassword[65];
    bool initialized;
    String deviceMac;  // Add MAC storage
    String mqttBaseTopic;
    String mqttTempTopic;
    String mqttHumTopic;

    bool loadKey();
    bool loadConfig();
    bool loadWiFiConfig();
    void generateRandomKey();

public:
    StorageManager();
    bool init(const char* defaultGroup = "group1", 
              const char* defaultSSID = "default_ssid", 
              const char* defaultPassword = "default_password");
    
    // Key management methods
    bool saveKey(const uint8_t* newKey);
    const uint8_t* getKey() const { return key; }
    
    // Config management methods
    bool saveConfig(const char* group);
    bool saveWiFiConfig(const char* ssid, const char* password);
    const char* getGroupName() const { return groupName; }
    const char* getWiFiSSID() const { return wifiSSID; }
    const char* getWiFiPassword() const { return wifiPassword; }
    
    bool isInitialized() const { return initialized; }
    const String& getDeviceMac() const { return deviceMac; }
    void setupTopics();
    const String& getMqttBaseTopic() const { return mqttBaseTopic; }
    String getMqttTempTopic();
    String getMqttHumTopic();
};
