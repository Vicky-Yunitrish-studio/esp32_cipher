#pragma once
#include <Arduino.h>
#include <DHT.h>

class DHTSensor {
    public:
        DHTSensor(uint8_t pin, uint8_t type); // Constructor
        void begin();                         // Initialize the sensor
        float getTemperature();               // Get the temperature
        float getHumidity();                  // Get the humidity

    private:
        DHT dht;                              // DHT sensor instance
        float temp;                           // Store temperature
        float hum;                            // Store humidity
};