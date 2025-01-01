#include <Arduino.h>
#include <WiFi.h>
class Connect {
private:
    String password;
public:
    String ssid;
    String status;
    int row;
    int col;
    int tryDelay = 500;
    int numberOfTries = 20;
    Connect();
    Connect(char *ssid, char *password);
    void setup();
    bool isConnected();
    void disConnect();
    void link();
};
