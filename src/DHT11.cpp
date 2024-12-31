#include "DHT11.h"

DHT11::DHT11(uint8_t pin) : pin(pin) {
    dht = new DHT(pin, DHT11_SENSOR);
}

void DHT11::setup() {
    dht->begin();
}

float DHT11::getTemperature() {
    float t = dht->readTemperature();
    return isnan(t) ? 0.0 : t;
}

float DHT11::getHumidity() {
    float h = dht->readHumidity();
    return isnan(h) ? 0.0 : h;
}
