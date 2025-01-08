#include "MQTT.h"

MQTT::MQTT() : encryptionEnabled(false) {
    encryptionQueue = xQueueCreate(10, sizeof(EncryptionJob));
    client.setClient(espClient);
}

void MQTT::setup(const char* server, int port) {
    client.setServer(DEFAULT_BROKER, DEFAULT_PORT);
    
    // Add TLS support for ESP32
    espClient.setInsecure();  // Skip certificate validation for now
    espClient.setHandshakeTimeout(30);  // Increase handshake timeout if needed
    
    // Set credentials using username_pw_set method
    // client.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
    
    client.disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));
}

bool MQTT::connect(const char* clientId) {
    if (!client.connected()) {
        if (client.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD)) {  // Add credentials here too
            Serial.println("MQTT Connected");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        return false;
    }
    return true;
}

void MQTT::enableEncryption(bool enable) {
    encryptionEnabled = enable;
    if (enable) {
        // Create encryption task
        xTaskCreatePinnedToCore(
            encryptionTaskFunction,
            "EncryptionTask",
            8192,  // Stack size
            this,
            1,    // Priority
            &encryptionTask,
            0     // Core ID
        );
    }
}

void MQTT::encryptionTaskFunction(void* parameter) {
    MQTT* mqtt = (MQTT*)parameter;
    EncryptionJob job;

    while (true) {
        if (xQueueReceive(mqtt->encryptionQueue, &job, portMAX_DELAY)) {
            if (job.isPublish) {
                // Encrypt and publish
                String encryptedData = mqtt->encryption.encrypt(job.data);
                mqtt->client.publish(job.topic.c_str(), encryptedData.c_str());
            } else {
                // Decrypt received data
                String decryptedData = mqtt->encryption.decrypt(job.data);
                // Handle decrypted data
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Prevent watchdog triggers
    }
}

void MQTT::publish(const char* topic, const char* payload) {
    if (encryptionEnabled) {
        String encryptedData = encryption.encrypt(String(payload));
        
        // 使用無冒號的MAC地址
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        
        String message = "{\"data\":\"" + encryptedData + "\",";
        message += "\"mac\":\"" + mac + "\",";
        message += "\"timestamp\":" + String(millis()) + "}";
        
        bool success = client.publish(topic, message.c_str());
        if (!success) {
            Serial.println("MQTT publish failed");
            return;
        }
    } else {
        client.publish(topic, payload);
    }
    client.loop();
}

void MQTT::loop() {
    if (client.connected()) {
        client.loop();
    }
}
