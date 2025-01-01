#include <Arduino.h>
#include <Timer.h>

void Timer::setup()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    recordedTime = time(nullptr);
    currentTime = time(nullptr);
}

void Timer::loop()
{
    struct tm tmp;
    if (getLocalTime(&tmp))
    {
        currentTime = mktime(&tmp);
    }
}

char* Timer::getTime()
{
    strncpy(timeStr, ctime(&currentTime) + 11, 8);
    timeStr[8] = '\0';
    return timeStr;
}

double Timer::getDiff(time_t& timer)
{
    double temp = difftime(currentTime, timer);
    timer = currentTime;
    return temp;
}

bool Timer::isTimeUp(time_t& timer, double interval)
{
    if (difftime(currentTime, timer) >= interval)
    {
        timer = currentTime;
        return true;
    }
    return false;
}