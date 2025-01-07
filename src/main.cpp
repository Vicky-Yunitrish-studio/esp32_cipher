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

#include "StorageManager.h"
StorageManager storage;

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Cipher Device Starting...");
  
  screen.setup();
  screen.display.clearDisplay();
  
  // Initialize storage manager
  screen.drawString(0, 0, "Loading config", 1, 0, 1);
  screen.display.display();
  if (!storage.init()) {
      screen.drawString(0, 16, "Storage init failed!", 1, 0, 1);
      screen.display.display();
      delay(2000);
  }

  // Initialize WiFi with config settings
  WiFi.begin(storage.getWiFiSSID(), storage.getWiFiPassword());
  
  // Rest of initialization
  light.setup();
  dhtSensor.begin();
  connect.setup();
  
  while (connect.numberOfTries) {
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
  mqtt.setup();
  delay(3000);

  // Set GPIO0 Boot button as input
  pinMode(btnGPIO, INPUT);

  // Show device info
  screen.display.clearDisplay();
  screen.drawString(0, 0, ("Device: " + storage.getDeviceMac()).c_str(), 1, 0, 1);  // Updated to use StorageManager
  screen.drawString(0, 8, ("Group: " + String(storage.getGroupName())).c_str(), 1, 0, 1);
  screen.display.display();
  delay(2000);
  
  mqtt.setup();

  // Create timer events
  timer.createEvent("display", 200);
  timer.createEvent("sensor", 2000);
  timer.createEvent("mqtt", 5000);
  timer.createEvent("time", 1000);
  timer.createEvent("led", 50); // 0.05 seconds
}
bool needDisplayUpdate = false;
void loop() {
  unsigned long currentMillis = millis();
  
  // 按鈕讀取和WiFi狀態更新
  btnState = digitalRead(btnGPIO);
  connect.link();  // 確保WiFi保持連接
  
  // 更新時間
  if (timer.getEvent("time")->isReady()) {
    timer.loop();  // 更新時間
    needDisplayUpdate = true;  // 強制更新顯示
  }
  
  // 定時讀取傳感器數據
  if (timer.getEvent("sensor")->isReady()) {
    temp = dhtSensor.getTemperature();
    hum = dhtSensor.getHumidity();
    needDisplayUpdate = true;
  }

  // LED更新
  if (connect.isConnected() && timer.getEvent("led")->isReady()) {
    light.update();
  }
  
  // Update MQTT publish to use StorageManager's topics
  if (connect.isConnected() && timer.getEvent("mqtt")->isReady()) {
    String tempStr = String(temp, 2);
    String humStr = String(hum, 2);
    
    mqtt.publish(storage.getMqttTempTopic().c_str(), tempStr.c_str());
    mqtt.publish(storage.getMqttHumTopic().c_str(), humStr.c_str());
  }

  // 顯示更新
  if (needDisplayUpdate && timer.getEvent("display")->isReady()) {
    screen.display.clearDisplay();
    
    // 獲取最新時間
    String currentTimeStr = timer.getTime();
    screen.drawString(0, 0, currentTimeStr.c_str(), 1, 0, 1);
    screen.drawString(connect.col, 0, connect.status.c_str(), 1, 0, 1);
    
    if (connect.isConnected()) {
      screen.drawString(0, 8, ("Group: " + String(storage.getGroupName())).c_str(), 1, 0, 1);
      screen.drawString(0, 16, ("SSID: " + connect.ssid).c_str(), 1, 0, 1);
    }
    
    screen.drawString(0, 32, ("TEMP:" + String(temp, 1) + "*C").c_str(), 1, 0, 1);
    screen.drawString(0, 48, ("HUM:" + String(hum, 1) + "%").c_str(), 1, 0, 1);
    
    screen.display.display();
    needDisplayUpdate = false;
  }

  // 按鈕處理
  if (btnState == LOW) {
    connect.disConnect();
    delay(200);
    needDisplayUpdate = true;
  }
}
