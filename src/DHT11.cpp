#include "DHT11.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type) : dht(pin, type), temp(0), hum(0) {}

void DHTSensor::begin() {
    dht.begin();
}

float DHTSensor::getTemperature() {
    temp = dht.readTemperature();
    if (isnan(temp)) {
        Serial.println("Failed to read temperature from DHT sensor!");
        return 0; // Return -1 if reading fails
    }
    return temp;
}

float DHTSensor::getHumidity() {
    hum = dht.readHumidity();
    if (isnan(hum)) {
        Serial.println("Failed to read humidity from DHT sensor!");
        return 0; // Return -1 if reading fails
    }
    return hum;
}
