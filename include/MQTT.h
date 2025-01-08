#pragma once
#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "Encryption.h"

class MQTT {
private:
    static constexpr const char* DEFAULT_BROKER = "81a9af6ef0a24070877e2fdb6ce5adb9.s1.eu.hivemq.cloud";
    static constexpr int DEFAULT_PORT = 8883;
    static constexpr const char* MQTT_USERNAME = "esp32-0001";
    static constexpr const char* MQTT_PASSWORD = "Esp320002";

    WiFiClientSecure espClient;
    PubSubClient client;
    Encryption encryption;
    bool encryptionEnabled;
    QueueHandle_t encryptionQueue;
    TaskHandle_t encryptionTask;
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_DELAY = 5000; // 5 seconds

    struct EncryptionJob {
        String data;
        String topic;
        bool isPublish;
    };

    static void encryptionTaskFunction(void* parameter);

public:
    MQTT();
    void setup(const char* server = "broker.hivemq.com", int port = 1883);
    bool connect(const char* clientId = "ESP32_Client");
    void publish(const char* topic, const char* payload);
    void loop();
    bool isConnected() { 
        if (!client.connected()) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > RECONNECT_DELAY) {
                lastReconnectAttempt = now;
                connect();
            }
        }
        return client.connected(); 
    }
    void enableEncryption(bool enable);
    static void callback(char* topic, byte* payload, unsigned int length);
};

#endif
