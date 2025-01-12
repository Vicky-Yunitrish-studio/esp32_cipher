#pragma once
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

class MQTT {
private:
    WiFiClientSecure espClient;
    PubSubClient client;
    const char* username;
    const char* password;
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_DELAY = 5000;

public:
    MQTT() : username(nullptr), password(nullptr) {
        client.setClient(espClient);
    }
    
    void setup(const char* broker, int port) {
        client.setServer(broker, port);
        espClient.setInsecure();
    }
    
    void setCredentials(const char* user, const char* pass) {
        username = user;
        password = pass;
    }
    
    bool connect(const char* clientId) {
        if (!client.connected()) {
            Serial.println("Connecting to MQTT...");
            if (client.connect(clientId, username, password)) {
                Serial.println("Connected to MQTT broker");
                return true;
            }
            Serial.println("Failed to connect to MQTT");
            return false;
        }
        return true;
    }
    
    bool publish(const char* topic, const char* payload) {
        if (!client.connected()) return false;
        return client.publish(topic, payload);
    }
    
    bool isConnected() {
        return client.connected();
    }
    
    void loop() {
        if (client.connected()) {
            client.loop();
        }
    }
};
