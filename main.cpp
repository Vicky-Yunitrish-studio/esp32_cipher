#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "StateMachine.h"
#include "Encryption.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER";

WiFiClient espClient;
PubSubClient client(espClient);
StateMachine stateMachine;
Encryption encryption;

// 初始金鑰（應該安全存儲）
const uint8_t initialKey[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

void setup() {
    Serial.begin(115200);
    encryption.init(initialKey);
    WiFi.begin(ssid, password);
}

void loop() {
    stateMachine.update();
    
    if (stateMachine.isInState(SystemState::RUNNING)) {
        // 準備發送的數據
        String message = "Hello, World!";
        // 加密
        String encrypted = encryption.encrypt(message);
        // 發送到MQTT
        client.publish("encrypted_topic", encrypted.c_str());
    }
    
    delay(100);
}
