#include "StateMachine.h"
#include <WiFi.h>

StateMachine::StateMachine() : currentState(SystemState::INIT), lastStateChange(0) {}

void StateMachine::setState(SystemState newState) {
    if (currentState != newState) {
        currentState = newState;
        lastStateChange = millis();
    }
}

SystemState StateMachine::getState() const {
    return currentState;
}

void StateMachine::update() {
    switch (currentState) {
        case SystemState::INIT:
            // 初始化完成後轉換到連接WiFi狀態
            setState(SystemState::CONNECTING_WIFI);
            break;
            
        case SystemState::CONNECTING_WIFI:
            // WiFi連接成功後轉換到連接MQTT狀態
            if (WiFi.status() == WL_CONNECTED) {
                setState(SystemState::CONNECTING_MQTT);
            }
            break;
            
        case SystemState::CONNECTING_MQTT:
            // MQTT連接成功後轉換到運行狀態
            if (true) {
                setState(SystemState::RUNNING);
            }
            break;
            
        case SystemState::RUNNING:
            // 正常運行狀態的處理
            break;
            
        case SystemState::ERROR:
            // 錯誤處理
            break;
    }
}

unsigned long StateMachine::getTimeInState() const {
    return millis() - lastStateChange;
}

bool StateMachine::isInState(SystemState state) const {
    return currentState == state;
}
