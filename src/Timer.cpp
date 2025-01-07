#include <Arduino.h>
#include <Timer.h>
#include <time.h>

bool TimerEvent::isReady() {
    if (!enabled) return false;
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        return true;
    }
    return false;
}

void TimerEvent::reset() {
    lastUpdate = millis();
}

Timer::Timer() : timeValid(false) {
    memset(&timeinfo, 0, sizeof(timeinfo));
    recordedTime = 0;
    currentTime = 0;
    memset(timeStr, 0, sizeof(timeStr));
}

Timer::~Timer() {
    for (auto event : events) {
        delete event;
    }
    events.clear();
}

void Timer::setup() {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void Timer::loop() {
    if (!getLocalTime(&timeinfo)) {
        timeValid = false;
        return;
    }
    timeValid = true;
}

char* Timer::getTime() {
    static char timeString[20];
    if (!timeValid) {
        strcpy(timeString, "Time not set");
        return timeString;
    }
    snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return timeString;
}

TimerEvent* Timer::createEvent(const String& name, unsigned long interval) {
    auto event = new TimerEvent(name, interval);
    events.push_back(event);
    return event;
}

TimerEvent* Timer::getEvent(const String& name) {
    for (auto event : events) {
        if (event->getName() == name) {
            return event;
        }
    }
    return nullptr;
}

void Timer::removeEvent(const String& name) {
    for (auto it = events.begin(); it != events.end(); ++it) {
        if ((*it)->getName() == name) {
            delete *it;
            events.erase(it);
            break;
        }
    }
}