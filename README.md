# ESP 32 加密傳輸 與 樹梅派解碼

[期末專題報告](https://docs.google.com/presentation/d/1dgE1UflbfET3RkeLtujjp4L4WDkh4RkqZj0S-r6HGY4/edit?usp=sharing)
組員

- 林昀佑
- 戴育琪
- 王郁琁

## 安裝

|    裝置名稱    |  裝置io  | esp32-GPIO |
|---------------|----------|------------|
|    sh1106     |    VCC   |    3.3v    |
|    sh1106     |    GND   |     GND    |
|    sh1106     |    SCL   |     22     |
|    sh1106     |    SDA   |     21     |

|    裝置名稱    |  裝置io  | esp32-GPIO |
|---------------|----------|------------|
|     dht11     |    VCC   |    3.3v    |
|     dht11     |    GND   |     GND    |
|     dht11     |   DATA   |     14     |

|    裝置名稱    |  裝置io  | esp32-GPIO |
|---------------|----------|------------|
|   IRReciver   |    VCC   |    3.3v    |
|   IRReciver   |    GND   |     GND    |
|   IRReciver   |   DATA   |     23     |

|    裝置名稱    |  裝置io  | esp32-GPIO |
|---------------|----------|------------|
| IRTrasnmitter |    VCC   |    3.3v    |
| IRTrasnmitter |    GND   |     GND    |
| IRTrasnmitter |   DATA   |     32     |
|               |          |            |

![esp32 nodemcu 32s pinout](ESP32-NODEMCU-ESP-32S-Kit-pinout-low-res-mischianti-1024x599.jpg)

## 功能

1. 加密的MQTT傳輸協定
2. DHT11, SSD1306, LED, IR
3. flutter app
4. server端在raspi
5. 加密協定 chacha20

## 架構

1. 語言:C++
2. 伺服端: raspbian-os
