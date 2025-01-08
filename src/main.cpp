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

// Add new includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Add these at the top with other includes
#include "esp_task_wdt.h"
#include "esp_system.h"

// 修改常量定義
#define CORE0_STACK_SIZE (16 * 1024)  // 16KB
#define CORE1_STACK_SIZE (16 * 1024)  // 16KB
#define WDT_TIMEOUT 30                // 30 seconds
#define LED_PIN 2                     // 使用GPIO2替代LED_BUILTIN

// Add new global variables
TaskHandle_t Core0Task;
TaskHandle_t Core1Task;
QueueHandle_t displayQueue;
QueueHandle_t sensorQueue;
volatile bool needDisplayUpdate = false;

// Data structures for passing information between cores
struct DisplayData {
    float temp;
    float hum;
    bool wifiConnected;
    String timeStr;
    String wifiStatus;
};

struct SensorData {
    float temp;
    float hum;
};

// Core 0 task - handles network and sensors
void Core0Tasks(void *parameter) {
    // 重要：移除esp_task_wdt_init，改用下面的配置
    esp_task_wdt_delete(NULL);
    vTaskDelay(pdMS_TO_TICKS(100));  // 給系統一些時間穩定

    unsigned long lastMQTTSend = 0;
    const unsigned long MQTT_INTERVAL = 10000; // 固定10秒間隔
    
    for(;;) {
        unsigned long currentMillis = millis();

        // WiFi檢查
        if (timer.getEvent("wifi")->isReady()) {
            if (!connect.isConnected()) {
                connect.link();
                mqtt.setup();  // 重新設置MQTT
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // MQTT處理改為精確的時間間隔
        if (connect.isConnected() && (currentMillis - lastMQTTSend >= MQTT_INTERVAL)) {
            if (mqtt.isConnected()) {
                // 只在有效數據時發送
                if (!isnan(temp) && !isnan(hum)) {
                    // Data will be encrypted in MQTT class's separate task
                    mqtt.publish(storage.getMqttTempTopic().c_str(), String(temp, 1).c_str());
                    mqtt.publish(storage.getMqttHumTopic().c_str(), String(hum, 1).c_str());
                    lastMQTTSend = currentMillis; // 更新最後發送時間
                }
            } else {
                Serial.println("Reconnecting MQTT...");
                mqtt.setup();
            }
        }

        // Add error checking for sensor
        if (timer.getEvent("sensor")->isReady()) {
            SensorData sensorData;
            sensorData.temp = dhtSensor.getTemperature();
            sensorData.hum = dhtSensor.getHumidity();

            // Check if values are valid before sending
            if (!isnan(sensorData.temp) && !isnan(sensorData.hum)) {
                if (xQueueSend(sensorQueue, &sensorData, 0) != pdTRUE) {
                    Serial.println("Failed to send sensor data to queue");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // Time update
        if (timer.getEvent("time")->isReady()) {
            timer.loop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // Always include a small delay
        vTaskDelay(pdMS_TO_TICKS(10));  // 確保任務有時間讓出CPU
    }
}

// Core 1 task - handles display and UI
void Core1Tasks(void *parameter) {
    // Disable watchdog for this core
    esp_task_wdt_init(WDT_TIMEOUT, false);

    // Initialize variables outside the loop
    static uint32_t lastDisplayUpdate = 0;
    static uint32_t lastTimeUpdate = 0;
    static uint32_t lastButtonTime = 0;
    static String lastTimeStr;
    static String lastWiFiStatus;

    for(;;) {
        esp_task_wdt_reset();
        uint32_t currentMillis = millis();

        // 時間更新檢查 - 每秒更新一次
        if (currentMillis - lastTimeUpdate >= 1000) {
            lastTimeUpdate = currentMillis;
            String currentTimeStr = timer.getTime();
            if (currentTimeStr != lastTimeStr) {
                lastTimeStr = currentTimeStr;
                needDisplayUpdate = true;
            }
        }

        // Sensor data handling with timeout
        SensorData sensorData;
        if (xQueueReceive(sensorQueue, &sensorData, pdMS_TO_TICKS(10)) == pdTRUE) {
            temp = sensorData.temp;
            hum = sensorData.hum;
            needDisplayUpdate = true;
        }

        // LED control with delay
        if (connect.isConnected() && timer.getEvent("led")->isReady()) {
            light.update();
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // Display update with rate limiting
        if (needDisplayUpdate && (currentMillis - lastDisplayUpdate > 50)) {  // 降低更新間隔到50ms
            lastDisplayUpdate = currentMillis;
            screen.display.clearDisplay();
            
            // 直接顯示當前時間，不進行比較
            screen.drawString(0, 0, timer.getTime(), 1, 0, 1);
            
            // WiFi狀態更新
            if (connect.status != lastWiFiStatus) {
                screen.drawString(connect.col, 0, connect.status.c_str(), 1, 0, 1);
                lastWiFiStatus = connect.status;
            }

            // 其他顯示內容
            if (connect.isConnected()) {
                screen.drawString(0, 8, ("Group: " + String(storage.getGroupName())).c_str(), 1, 0, 1);
                screen.drawString(0, 16, ("SSID: " + connect.ssid).c_str(), 1, 0, 1);
            }

            screen.drawString(0, 32, ("TEMP:" + String(temp, 1) + "*C").c_str(), 1, 0, 1);
            screen.drawString(0, 48, ("HUM:" + String(hum, 1) + "%").c_str(), 1, 0, 1);

            screen.display.display();
            needDisplayUpdate = false;
        }

        // Button handling
        btnState = digitalRead(btnGPIO);
        if (btnState == LOW && (currentMillis - lastButtonTime > 500)) {
            lastButtonTime = currentMillis;
            connect.disConnect();
            needDisplayUpdate = true;
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Cipher Device Starting...");

  // 正確的WDT配置順序
  disableLoopWDT();
  vTaskDelay(pdMS_TO_TICKS(100));
  
  // 初始化LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // 初始化顯示
  screen.setup();
  screen.display.clearDisplay();
  screen.display.display();
  
  // Initialize storage manager
  screen.drawString(0, 0, "Loading config", 1, 0, 1);
  screen.display.display();
  if (!storage.init()) {
      screen.drawString(0, 16, "Storage init failed!", 1, 0, 1);
      screen.display.display();
      vTaskDelay(pdMS_TO_TICKS(2000));
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

  // Initialize encryption
  mqtt.enableEncryption(true);

  // Set GPIO0 Boot button as input
  pinMode(btnGPIO, INPUT);

  // Show device info
  screen.display.clearDisplay();
  screen.drawString(0, 0, ("Device: " + storage.getDeviceMac()).c_str(), 1, 0, 1);  // Updated to use StorageManager
  screen.drawString(0, 8, ("Group: " + String(storage.getGroupName())).c_str(), 1, 0, 1);
  screen.display.display();

  mqtt.setup();

  // Create queues with error checking
  displayQueue = xQueueCreate(5, sizeof(DisplayData));
  sensorQueue = xQueueCreate(10, sizeof(SensorData));
  if (!sensorQueue) {
      Serial.println("Failed to create sensor queue");
      return;
  }

  if (displayQueue == NULL || sensorQueue == NULL) {
      Serial.println("Error creating queues");
      ESP.restart();
  }

  // Create tasks with increased stack size and proper priorities
  BaseType_t core0Created = xTaskCreatePinnedToCore(
      Core0Tasks,
      "Core0Tasks",
      CORE0_STACK_SIZE,
      NULL,
      1,  // 降低優先級
      &Core0Task,
      0
  );

  if (core0Created != pdPASS) {
      Serial.println("Core 0 task creation failed");
      ESP.restart();
  }

  BaseType_t core1Created = xTaskCreatePinnedToCore(
      Core1Tasks,
      "Core1Tasks",
      CORE1_STACK_SIZE,
      NULL,
      1,  // 相同優先級
      &Core1Task,
      1
  );

  if (core1Created != pdPASS) {
      Serial.println("Core 1 task creation failed");
      ESP.restart();
  }

  // Create timer events (keep existing timing)
  timer.createEvent("display", 1000);  // 1秒
  timer.createEvent("sensor", 2000);   // 2秒
  timer.createEvent("time", 1000);     // 1秒
  timer.createEvent("wifi", 5000);     // 5秒
  timer.createEvent("led", 200);       // 200ms
  // 移除mqtt事件，因為我們現在使用精確的時間控制

  // Disable watchdog for main loop
  esp_task_wdt_init(WDT_TIMEOUT, false);

  Serial.println("Setup complete");
}

void loop() {
    vTaskDelete(NULL);  // Remove the current task
}
