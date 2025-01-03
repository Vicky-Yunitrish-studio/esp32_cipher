#include <Adafruit_SH1106.h>
#include <Screen.h>
Adafruit_SH1106 dp(-1);
Screen screen(dp);

#include <LightTest.h>
LightTest light(LED_BUILTIN);

#include <Timer.h>
Timer timer = Timer();
time_t ledTimer = 0;

#include <DHT11.h>
DHTSensor dhtSensor(14, DHT11);
float temp = 0;
float hum = 0;

#include <Connect.h>
Connect connect = Connect();
int btnGPIO = 0;
int btnState = false;

#include "MQTT.h"
MQTT mqtt;

#include "Encryption.h"
#include "KeyManager.h"

#include <WiFi.h>

// Helper functions
String getDeviceMac() {
    return WiFi.macAddress();
}

// MQTT settings
const char* GROUP_NAME = "group1";  // 可以改成從設定檔讀取
String MQTT_BASE_TOPIC;
String TEMP_TOPIC;
String HUM_TOPIC;

void setupTopics() {
    String mac = getDeviceMac();
    mac.replace(":", "");
    MQTT_BASE_TOPIC = String("esp32/") + GROUP_NAME + "/" + mac;
    TEMP_TOPIC = MQTT_BASE_TOPIC + "/temp";  // 縮短名稱
    HUM_TOPIC = MQTT_BASE_TOPIC + "/hum";    // 縮短名稱
}

// Global objects
Encryption encryption;
KeyManager keyManager;

time_t mqttTimer = 0;

void setup() {
  Serial.begin(115200);
  screen.setup();
  screen.display.clearDisplay();
  screen.drawString(0, 0, "setting up devices", 1, 0, 1);
  screen.display.display();
  light.setup();
  dhtSensor.begin();
  connect.setup();
  delay(1000);

  while (connect.numberOfTries)
  {
    connect.link();
    screen.display.clearDisplay();
    screen.drawString(connect.col, connect.row, connect.status.c_str(), 1, 0, 1);
    if (connect.isConnected()) break;
    screen.display.display();
  }
  
  
  screen.display.clearDisplay();
  screen.drawString(0, 0, "Fetching time", 1, 0, 1);
  screen.display.display();
  timer.setup();
  
  
  screen.display.clearDisplay();
  screen.drawString(0, 0, "Loading key", 1, 0, 1);
  screen.display.display();
  // Initialize key manager and encryption
  if (!keyManager.init()) {
    screen.drawString(0, 0, "Key init failed!", 1, 0, 1);
    screen.display.display();
    delay(2000);
  }
  encryption.init(keyManager.getKey());
  encryption.setConstants(TEMP_TOPIC.c_str());
  
  mqtt.setup();
  delay(3000);

  // Set GPIO0 Boot button as input
  pinMode(btnGPIO, INPUT);

  // Setup MQTT topics after WiFi connection
  setupTopics();
  
  // Initialize encryption with topic
  if (!keyManager.init()) {
      screen.drawString(0, 0, "Key init failed!", 1, 0, 1);
      screen.display.display();
      delay(2000);
  }
  encryption.init(keyManager.getKey());
  encryption.setDeviceMac(getDeviceMac());  // Set MAC in nonce
  encryption.setConstants(TEMP_TOPIC.c_str());
  
  // Show device info
  screen.display.clearDisplay();
  screen.drawString(0, 0, ("Device: " + getDeviceMac()).c_str(), 1, 0, 1);
  screen.display.display();
  delay(2000);
  
  mqtt.setup();
}

void loop() {
  screen.display.clearDisplay();
  btnState = digitalRead(btnGPIO);
  timer.loop();
  screen.drawString(connect.col, 0, connect.status.c_str(), 1, 0, 1);
  screen.drawString(0, 0, timer.getTime(), 1, 0, 1);
  if (connect.isConnected()) {
    screen.drawString(0, 16, ("SSID: " + connect.ssid).c_str(), 1, 0, 1);
    if (timer.isTimeUp(ledTimer,0.05)) light.update();
  }
  /*---------------------------------------------------------------------------*/
  temp = dhtSensor.getTemperature();
  hum = dhtSensor.getHumidity();
  screen.drawString(0, 32, ("TEMP:" + String(temp) + "*C").c_str(), 1, 0, 1);
  screen.drawString(0, 48, ("HUM:" + String(hum) + "%").c_str(), 1, 0, 1);
  
  if (connect.isConnected() && timer.isTimeUp(mqttTimer, 5)) {  // Publish every 5 seconds
    // Encrypt and publish temperature
    String encryptedTemp = encryption.encrypt(String(temp));
    mqtt.publish(TEMP_TOPIC.c_str(), encryptedTemp.c_str());
    
    // Update constants for humidity
    encryption.setConstants(HUM_TOPIC.c_str());
    String encryptedHum = encryption.encrypt(String(hum));
    mqtt.publish(HUM_TOPIC.c_str(), encryptedHum.c_str());
    
    // Reset constants for next temperature reading
    encryption.setConstants(TEMP_TOPIC.c_str());
    
    mqttTimer = timer.currentTime;
  }
  /*---------------------------------------------------------------------------*/
  if (btnState == LOW) {
    // connect.isConnected() ? 
    connect.disConnect()
    // :connect.link()
    ;
    delay(200);
  }
  screen.display.display();
}
