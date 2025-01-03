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
#include "ConfigManager.h"

// Helper functions
String getDeviceMac() {
    return WiFi.macAddress();
}

// MQTT settings
String MQTT_BASE_TOPIC;
String TEMP_TOPIC;
String HUM_TOPIC;

// Global objects
Encryption encryption;
KeyManager keyManager;
ConfigManager configManager;

void setupTopics() {
    String mac = getDeviceMac();
    mac.replace(":", "");
    MQTT_BASE_TOPIC = String("esp32/") + configManager.getGroupName() + "/" + mac;
    TEMP_TOPIC = MQTT_BASE_TOPIC + "/temp";
    HUM_TOPIC = MQTT_BASE_TOPIC + "/hum";
}

time_t mqttTimer = 0;

void setup() {
  Serial.begin(115200);
  screen.setup();
  screen.display.clearDisplay();
  
  // Initialize config manager first
  screen.drawString(0, 0, "Loading config", 1, 0, 1);
  screen.display.display();
  if (!configManager.init()) {
      screen.drawString(0, 16, "Config init failed!", 1, 0, 1);
      screen.display.display();
      delay(2000);
  }

  // Initialize WiFi with config settings
  WiFi.begin(configManager.getWiFiSSID(), configManager.getWiFiPassword());
  
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

// 增加狀態控制變量
bool needProcessTemp = false;
bool needProcessHum = false;

// 添加常量定義
const int MAX_ENCRYPTION_STEPS_PER_LOOP = 2;  // 每次循環最多執行的加密步驟數

// 添加顯示相關變量
String lastTimeStr;
String lastTempStr;
String lastHumStr;
String lastEncryptionProgress;
bool needDisplayUpdate = false;

void loop() {
  // 讀取所有輸入但不立即顯示
  btnState = digitalRead(btnGPIO);
  timer.loop();
  
  // 檢查是否需要更新各個顯示元素
  String currentTimeStr = timer.getTime();
  if (currentTimeStr != lastTimeStr) {
    lastTimeStr = currentTimeStr;
    needDisplayUpdate = true;
  }
  
  String currentTempStr = String(dhtSensor.getTemperature());
  String currentHumStr = String(dhtSensor.getHumidity());
  if (currentTempStr != lastTempStr || currentHumStr != lastHumStr) {
    lastTempStr = currentTempStr;
    lastHumStr = currentHumStr;
    temp = currentTempStr.toFloat();
    hum = currentHumStr.toFloat();
    needDisplayUpdate = true;
  }

  // 只在需要時才清除和更新顯示
  if (needDisplayUpdate) {
    screen.display.clearDisplay();
    
    // 更新所有顯示內容
    screen.drawString(0, 0, lastTimeStr.c_str(), 1, 0, 1);
    screen.drawString(connect.col, 0, connect.status.c_str(), 1, 0, 1);
    
    if (connect.isConnected()) {
      screen.drawString(0, 16, ("SSID: " + connect.ssid).c_str(), 1, 0, 1);
    }
    
    screen.drawString(0, 32, ("TEMP:" + lastTempStr + "*C").c_str(), 1, 0, 1);
    screen.drawString(0, 48, ("HUM:" + lastHumStr + "%").c_str(), 1, 0, 1);
    
    needDisplayUpdate = false;
  }

  // LED更新不影響螢幕顯示
  if (connect.isConnected() && timer.isTimeUp(ledTimer, 0.05)) {
    light.update();
  }
  
  // 加密相關操作
  if (connect.isConnected() && timer.isTimeUp(mqttTimer, 10)) {
    needProcessTemp = true;
    mqttTimer = timer.currentTime;
  }

  // 處理加密，使用單獨的進度顯示更新
  if (encryption.isEncryptionInProgress()) {
    String currentProgress = String(encryption.getProgress()) + "%";
    if (currentProgress != lastEncryptionProgress) {
      lastEncryptionProgress = currentProgress;
      // 只更新進度行，不清除整個螢幕
      screen.drawString(0, 56, 
        (String(needProcessTemp ? "Temp" : "Hum") + " Encrypting: " + 
         currentProgress).c_str(), 
        1, 0, 1);
    }
    
    // 加密處理邏輯保持不變
    int encryptionSteps = 0;
    while (encryptionSteps < MAX_ENCRYPTION_STEPS_PER_LOOP) {
      if (!encryption.isEncryptionInProgress()) {
        if (needProcessTemp) {
          encryption.setConstants(TEMP_TOPIC.c_str());
          encryption.startChunkedEncryption(String(temp));
        }
        break;
      }

      if (!encryption.blockReady) {
        encryption.prepareNextBlock();
        encryptionSteps++;
      } else if (!encryption.processNextChunk()) {
        String encryptedData = encryption.finishEncryption();
        
        if (needProcessTemp) {
          mqtt.publish(TEMP_TOPIC.c_str(), encryptedData.c_str());
          needProcessTemp = false;
          needProcessHum = true;
        } else if (needProcessHum) {
          mqtt.publish(HUM_TOPIC.c_str(), encryptedData.c_str());
          needProcessHum = false;
        }
        
        if (needProcessHum) {
          encryption.setConstants(HUM_TOPIC.c_str());
          encryption.startChunkedEncryption(String(hum));
        }
        break;
      }
      encryptionSteps++;
    }
  }

  // 按鈕處理
  if (btnState == LOW) {
    connect.disConnect();
    delay(200);
    needDisplayUpdate = true;  // 確保狀態改變後更新顯示
  }

  // 每次循環結束時更新顯示
  screen.display.display();
}
