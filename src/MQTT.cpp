#include "MQTT.h"

MQTT::MQTT() : client(espClient) {}

void MQTT::setup(const char* server, int port) {
    client.setServer(server, port);
}

bool MQTT::connect(const char* clientId) {
    if (!client.connected()) {
        if (client.connect(clientId)) {
            return true;
        }
        delay(500);
        return false;
    }
    return true;
}

void MQTT::publish(const char* topic, const char* payload) {
    if (connect()) {
        client.publish(topic, payload);
    }
}

void MQTT::loop() {
    if (!client.connected()) {
        connect();
    }
    client.loop();
}
