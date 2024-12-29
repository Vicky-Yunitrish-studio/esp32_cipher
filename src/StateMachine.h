// #pragma once
// #include <Arduino.h>

// enum class SystemState {
//     INIT,
//     CONNECTING_WIFI,
//     CONNECTING_MQTT,
//     RUNNING,
//     ERROR
// };

// class StateMachine {
// private:
//     SystemState currentState;
//     unsigned long lastStateChange;
    
// public:
//     StateMachine();
//     void setState(SystemState newState);
//     SystemState getState() const;
//     void update();
//     unsigned long getTimeInState() const;
//     bool isInState(SystemState state) const;
// };
