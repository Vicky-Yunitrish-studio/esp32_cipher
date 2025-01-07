#pragma once
class Adafruit_SH1106;

class Screen {
private:
    long long counter;
    int16_t x;
    int16_t y;
    int16_t xDir;
    int16_t yDir;
    
public:
    Adafruit_SH1106& display;
    Screen(Adafruit_SH1106& disp);
    void setup();
    void loop();
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size);
};