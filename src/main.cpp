#include <WiFi.h>
#include <time.h>

const char *ssid = "Yun";
const char *password = "0937565253";

const char* ntpServer = "tock.stdtime.gov.tw";
const long gmtOffset_sec = 8 * 3600;    // 台灣 GMT+8
const int daylightOffset_sec = 0; 
time_t recordedTime = 0;
time_t currentTime = 0;

int btnGPIO = 0;
int btnState = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
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
  while (true) {

    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL: Serial.println("[WiFi] SSID not found"); break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
        return;
        break;
      case WL_CONNECTION_LOST: Serial.println("[WiFi] Connection was lost"); break;
      case WL_SCAN_COMPLETED:  Serial.println("[WiFi] Scan is completed"); break;
      case WL_DISCONNECTED:    Serial.println("[WiFi] WiFi is disconnected"); break;
      case WL_CONNECTED:
        Serial.println("[WiFi] WiFi is connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        struct tm tmp;
        if (getLocalTime(&tmp)) {
          recordedTime = mktime(&tmp);
          Serial.printf("Initial Recorded Time: %ld\n", recordedTime);
        } else {
          Serial.println("Failed to obtain initial time");
        }
        return;
        break;
      default:
        Serial.print("[WiFi] WiFi Status: ");
        Serial.println(WiFi.status());
        break;
    }
    delay(tryDelay);

    if (numberOfTries <= 0) {
      Serial.print("[WiFi] Failed to connect to WiFi!");
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return;
    } else {
      numberOfTries--;
    }
  }
}
bool light_status = false;
void loop() {
  btnState = digitalRead(btnGPIO);

  struct tm tmp;
  if (getLocalTime(&tmp)) {
    currentTime = mktime(&tmp);
  } else {
    Serial.println("Failed to obtain current time");
    delay(1000);
    return;
  }

  if (difftime(currentTime, recordedTime) >= 5) {
    recordedTime = currentTime;
    if (WiFi.status() == WL_CONNECTED) {
      if (light_status) {
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else {
        digitalWrite(LED_BUILTIN, LOW);
      }
      light_status = !light_status;
    }
  }
  
  if (btnState == LOW) {
    // Disconnect from WiFi
    Serial.println("[WiFi] Disconnecting from WiFi!");
    // This function will disconnect and turn off the WiFi (NVS WiFi data is kept)
    if (WiFi.disconnect(true, false)) {
      Serial.println("[WiFi] Disconnected from WiFi!");
    }
    delay(1000);
  }
}