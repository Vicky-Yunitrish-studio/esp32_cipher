#!/bin/bash

echo "正在檢查ESP32設備..."

# 檢查是否有ESP32設備連接 (CP210x或CH340)
if ! (lsusb | grep -iE "CP210x|CH340" > /dev/null); then
    echo "未檢測到ESP32設備,請確認:"
    echo "1. ESP32已正確連接"
    echo "2. 在Windows PowerShell(管理員)中執行:"
    echo "   usbipd list"
    echo "   usbipd wsl attach --busid <BUSID>"
    exit 1
fi

echo "ESP32設備已連接"
echo "設備位置: $(ls /dev/ttyUSB*)"
