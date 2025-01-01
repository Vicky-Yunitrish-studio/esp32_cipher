#include <Arduino.h>
class Timer {
private:
    const char *ntpServer = "tock.stdtime.gov.tw";
    const long gmtOffset_sec = 8 * 3600; // 台灣 GMT+8
    const int daylightOffset_sec = 0;
    time_t recordedTime = 0;
public:
    char timeStr[9];
    time_t currentTime = 0;
    void setup();
    void loop();
    char* getTime();
    double getDiff(time_t& timer);
    bool isTimeUp(time_t& timer, double interval);
};