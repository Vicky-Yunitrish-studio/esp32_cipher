#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTT {
private:
    WiFiClient espClient;
    PubSubClient client;
    const char* mqtt_server = "broker.hivemq.com";  // HiveMQ broker
    const int mqtt_port = 1883;
    const char* client_id = "ESP32_Client";
    const char* temp_topic = "yunitrish/esp32/temperature";  // Using unique topic path
    const char* hum_topic = "yunitrish/esp32/humidity";      // Using unique topic path

public:
    MQTT() : client(espClient) {}

    void setup() {
        client.setServer(mqtt_server, mqtt_port);
    }

    bool connect() {
        if (!client.connected()) {
            if (client.connect(client_id)) {
                return true;
            }
            delay(500);
            return false;
        }
        return true;
    }

    void publish(float temperature, float humidity) {
        if (!connect()) return;
        
        client.publish(temp_topic, String(temperature).c_str());
        client.publish(hum_topic, String(humidity).c_str());
    }

    void loop() {
        if (!client.connected()) {
            connect();
        }
        client.loop();
    }
};

#endif
