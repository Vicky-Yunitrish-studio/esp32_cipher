#include <WiFi.h>
#include <Connect.h>

Connect::Connect(char *ssid, char *password)
{
    this->ssid = ssid;
    this->password = password;
    // status = "Disconnected";
}

Connect::Connect()
{
    ssid = "Yun";
    password = "0937565253";
    // status = "Disconnected";
}

void Connect::setup()
{
    WiFi.begin(ssid.c_str(), password.c_str());
    status = "Connecting to " + String(ssid);
    this->col = (128-status.length()*6)/2;
    this->row = 32;
}

void Connect::link() {
    switch (WiFi.status()) {
        case WL_NO_SSID_AVAIL:
        {
            status = "No SSID Available";
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            break;
        }
        case WL_CONNECT_FAILED:
        {
            status = "Connection Failed";
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            break;
        }
        case WL_CONNECTION_LOST:
        {
            status = "Connection Lost";
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            break;
        }
        case WL_SCAN_COMPLETED:
        {
            status = "Scan is completed";
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            break;
        }
        case WL_DISCONNECTED:
        {
            status = "Disconnected";
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            break;
        }
        case WL_CONNECTED:
        {
            status = "Connected";
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            return;
        }
        default:
        {   
            status = "Unknown Error"+String(WiFi.status());
            this->col = (128-status.length()*6)/2;
            this->row = 32;
            break;
        }
    }
    if (numberOfTries <= 0) {
        WiFi.disconnect();
        status = "Failed to connect";
        this->col = (128-status.length()*6)/2;
        this->row = 32;
        return;
    }
    else {
        status = "Connecting:" + String(numberOfTries);
        this->col = (128-status.length()*6)/2;
        this->row = 32;
        numberOfTries--;
    }
    delay(tryDelay);
}

bool Connect::isConnected() {
    bool result = WiFi.status() == WL_CONNECTED;
    if (result) {
        status = "Connected";
        this->col = (128-status.length()*6);
        this->row = 32;
    }
    return result;
}

void Connect::disConnect() {
    WiFi.disconnect();
    status = "Disconnected";
    this->col = (128-status.length()*6)/2;
    this->row = 32;
}