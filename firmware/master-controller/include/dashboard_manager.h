#pragma once

// Blynk Virtual Pins
#define VPIN_B1_LDR_ENABLE      V5
#define VPIN_B1_LDR_ENABLE_NUM  5
#define VPIN_B1_MOTION_TIMEOUT      V6
#define VPIN_B1_MOTION_TIMEOUT_NUM  6

#define VPIN_GLOBAL_TEMPERATURE      V7
#define VPIN_GLOBAL_TEMPERATURE_NUM  7
#define VPIN_GLOBAL_HUMIDITY         V8
#define VPIN_GLOBAL_HUMIDITY_NUM     8

#define VPIN_LR_LDR_ENABLE      V15
#define VPIN_LR_LDR_ENABLE_NUM  15
#define VPIN_LR_MOTION_TIMEOUT      V16
#define VPIN_LR_MOTION_TIMEOUT_NUM  16

void initDashboard();
void updateDashboard();
void notifyDeviceStateChange(
    uint8_t nodeID,
    uint8_t deviceID,
    bool newState
);
void updateAllBlynkWidgets();
void setDeviceMode(uint8_t nodeID, uint8_t deviceID, uint8_t newMode);