#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include "Screen.h"

Adafruit_SH1106 s1 = Adafruit_SH1106(-1);
long long counter = 0;
int16_t x = 0;
int16_t y = 0;
int16_t xDir = 1;
int16_t yDir = 1;

Screen::Screen(Adafruit_SH1106& disp) : 
    display(disp),
    counter(0),
    x(0),
    y(0),
    xDir(1),
    yDir(1) {
}

void Screen::screenSetup() {
    display.begin();
    display.display();
    delay(1000);
}

void Screen::drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {
    display.setTextSize(size);
    display.setTextColor(color);
    display.setCursor(x, y);
    display.println(str);
}

void Screen::screenLoop() {
    if (x>126 || x<0) xDir *= -1;
    if (y>62 || y<0) yDir *= -1;
    x += xDir;
    y += yDir;
    display.fillScreen(0);
    if (counter%2==0) {
        display.drawCircle(x, y, 3, 1);
    }
    else {
        display.fillCircle(x, y, 3, 1);
    }
    display.drawChar(120, 0, '0'+counter%10, 1, 0, 1);
    display.drawChar(112, 0, '0'+(counter/10)%10, 1, 0, 1);
    display.drawChar(104, 0, '0'+(counter/100)%10, 1, 0, 1);
    counter++;
    display.display();
}