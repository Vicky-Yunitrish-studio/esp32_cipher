#include <WiFi.h>
#include <time.h>
#include <Adafruit_SH1106.h>
#include "Screen.h"
#include "LightTest.h"

const char *ssid = "朕的流量_與民共享";
const char *password = "V204123456789";

const char *ntpServer = "tock.stdtime.gov.tw";
const long gmtOffset_sec = 8 * 3600; // 台灣 GMT+8
const int daylightOffset_sec = 0;
time_t recordedTime = 0;
time_t currentTime = 0;
Adafruit_SH1106 dp(-1);
Screen screen(dp);
int btnGPIO = 0;
int btnState = false;

#define DHTPIN 14
#define DHTTYPE DHT11
#include <DHT.h>
DHT dht(DHTPIN, DHTTYPE);
float temp = 0;
float hum = 0;

LightTest light(LED_BUILTIN);

void setup()
{
  screen.setup();
  light.setup();
  dht.begin();
  Serial.begin(115200);
  delay(10);

  // Set GPIO0 Boot button as input
  pinMode(btnGPIO, INPUT);

  // We start by connecting to a WiFi network
  // To debug, please enable Core Debug Level to Verbose

  Serial.println();
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  // Auto reconnect is set true as default
  // To set auto connect off, use the following function
  //    WiFi.setAutoReconnect(false);

  // Will try for about 10 seconds (20x 500ms)
  int tryDelay = 500;
  int numberOfTries = 20;

  // Wait for the WiFi event
  while (true)
  {

    switch (WiFi.status())
    {
    case WL_NO_SSID_AVAIL:
    {
      Serial.println("[WiFi] SSID not found");
      screen.display.clearDisplay();
      screen.drawString(0, 48, "SSID not found", 1, 0, 1);
      screen.display.display();
      break;
    }
    case WL_CONNECT_FAILED:
    {
      Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
      screen.display.clearDisplay();
      screen.drawString(0, 48, "WiFi connected Failed!", 1, 0, 1);
      screen.display.display();
      break;
    }
    case WL_CONNECTION_LOST:
    {
      Serial.print("[WiFi] Connection was lost");
      screen.display.clearDisplay();
      screen.drawString(0, 48, "WiFi Connection was lost", 1, 0, 1);
      screen.display.display();
      break;
    }
    case WL_SCAN_COMPLETED:
    {
      Serial.print("[WiFi] Scan is completed");
      screen.display.clearDisplay();
      screen.drawString(0, 48, "Scan is completed", 1, 0, 1);
      screen.display.display();
      break;
    }
    case WL_DISCONNECTED:
    {
      Serial.print("[WiFi] WiFi is disconnected");
      screen.display.clearDisplay();
      screen.drawString(0, 48, "WiFi is disconnected", 1, 0, 1);
      screen.display.display();
      break;
    }
    case WL_CONNECTED:
    {
      Serial.println("[WiFi] WiFi is connected!");
      Serial.print("[WiFi] IP address: ");
      Serial.println(WiFi.localIP());
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      struct tm tmp;
      if (getLocalTime(&tmp))
      {
        recordedTime = mktime(&tmp);
        Serial.printf("Initial Recorded Time: %ld\n", recordedTime);
      }
      else
      {
        Serial.println("Failed to obtain initial time");
      }
      return;
      break;
    }
    default:
    {
      Serial.print("[WiFi] WiFi Status: ");
      Serial.println(WiFi.status());
      screen.display.clearDisplay();
      screen.drawString(0, 48, "Wifi Unknown Error", 1, 0, 1);
      screen.display.display();
      break;
    }
    }
    delay(tryDelay);

    if (numberOfTries <= 0)
    {
      Serial.print("[WiFi] Failed to connect to WiFi!");
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return;
    }
    else
    {
      numberOfTries--;
    }
  }
}

void loop()
{
  screen.display.clearDisplay();
  btnState = digitalRead(btnGPIO);

  struct tm tmp;
  if (getLocalTime(&tmp))
  {
    currentTime = mktime(&tmp);
    char x[9];
    strncpy(x, ctime(&currentTime) + 11, 8);
    x[8] = '\0';
    screen.drawString(0, 0, x, 1, 0, 1);
  }
  else
  {
    Serial.println("Failed to obtain current time");
    delay(1000);
    return;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    screen.drawString(74, 0, "Connected", 1, 0, 1);
    screen.drawString(74, 16, "SSID: ", 1, 0, 1);
    screen.drawString(74, 32, WiFi.SSID().c_str(), 1, 0, 1);

    temp = dht.readTemperature();
    hum = dht.readHumidity();
    screen.drawString(0, 16, ("TEMP:" + String(temp) + "*C").c_str(), 1, 0, 1);
    screen.drawString(0, 32, ("HUM:" + String(hum) + "%").c_str(), 1, 0, 1);
  }
  else
  {
    screen.drawString(56, 0, "Disconnected", 1, 0, 1);
  }

  if (difftime(currentTime, recordedTime) >= 0.2)
  {
    recordedTime = currentTime;
    if (WiFi.status() == WL_CONNECTED)
    {
      light.update();
    }
  }

  if (btnState == LOW)
  {
    // Disconnect from WiFi
    Serial.println("[WiFi] Disconnecting from WiFi!");
    // This function will disconnect and turn off the WiFi (NVS WiFi data is kept)
    if (WiFi.disconnect(true, false))
    {
      Serial.println("[WiFi] Disconnected from WiFi!");
    }
    delay(1000);
  }
  screen.display.display();
}
