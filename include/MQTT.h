#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTT {
private:
    WiFiClient espClient;
    PubSubClient client;

public:
    MQTT();
    void setup(const char* server = "broker.hivemq.com", int port = 1883);
    bool connect(const char* clientId = "ESP32_Client");
    void publish(const char* topic, const char* payload);
    void loop();
    bool isConnected() { return client.connected(); }
};

#endif
