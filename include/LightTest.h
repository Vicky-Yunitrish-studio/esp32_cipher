#pragma once
#include <Arduino.h>
#include <Wire.h>

class LightTest {
    private:
        int pin;
        int state;
    public:
        LightTest(int pin);
        void update();
        void setup();
        void loop();
};