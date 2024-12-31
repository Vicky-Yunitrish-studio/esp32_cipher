#include <Arduino.h>
#include <Wire.h>
#include "LightTest.h"


int pin = LED_BUILTIN;
int state = LOW;

LightTest::LightTest(int pin) {
    this->pin = pin;
    this->state = LOW;
}

void LightTest::setup() {
    pinMode(pin, OUTPUT);
}

void LightTest::update() {
    digitalWrite(pin, state);
    state = !state;
}

void LightTest::loop() {
    update();
    delay(200);
    update();
    delay(200);
    update();
    delay(200);
    update();
    delay(200);
    update();
    delay(2000);
    update();
    delay(2000);
}