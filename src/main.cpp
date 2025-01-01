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

int btnGPIO = 0;
int btnState = false;

#include <Connect.h>
Connect connect = Connect();

#include "MQTT.h"
MQTT mqtt;

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
  mqtt.setup();
  delay(3000);

  // Set GPIO0 Boot button as input
  pinMode(btnGPIO, INPUT);
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
    mqtt.publish(temp, hum);
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
