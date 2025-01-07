#pragma once
#include <Arduino.h>
#include <vector>

class TimerEvent {
private:
    unsigned long interval;
    unsigned long lastUpdate;
    bool enabled;
    String name;

public:
    TimerEvent(const String& eventName, unsigned long intervalMs) 
        : interval(intervalMs), lastUpdate(0), enabled(true), name(eventName) {}
    
    bool isReady();
    void reset();
    void setInterval(unsigned long newInterval) { interval = newInterval; }
    void setEnabled(bool state) { enabled = state; }
    bool isEnabled() const { return enabled; }
    const String& getName() const { return name; }
};

class Timer {
private:
    const char *ntpServer = "tock.stdtime.gov.tw";
    const long gmtOffset_sec = 8 * 3600; // 台灣 GMT+8
    const int daylightOffset_sec = 0;
    time_t recordedTime = 0;
    std::vector<TimerEvent*> events;
    bool timeValid;
    struct tm timeinfo;
public:
    Timer();   // Add constructor declaration
    ~Timer();  // Add destructor declaration
    char timeStr[9];
    time_t currentTime = 0;
    void setup();
    void loop();
    char* getTime();
    double getDiff(time_t& timer);
    bool isTimeUp(time_t& timer, double interval);
    
    // Event management
    TimerEvent* createEvent(const String& name, unsigned long interval);
    TimerEvent* getEvent(const String& name);
    void removeEvent(const String& name);
};