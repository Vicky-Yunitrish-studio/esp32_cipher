#ifndef DHT11_H
#define DHT11_H
#define DHT11_SENSOR 11
#include <Arduino.h>
#include <DHT.h>

class DHT11 {
private:
    DHT* dht;
    uint8_t pin;

public:
    DHT11(uint8_t pin);
    void setup();
    float getTemperature();
    float getHumidity();
};

#endif