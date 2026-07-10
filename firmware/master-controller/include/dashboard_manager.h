#pragma once

// Blynk Virtual Pins
#define VPIN_B1_LDR_ENABLE      V5
#define VPIN_B1_LDR_ENABLE_NUM  5
#define VPIN_B1_MOTION_TIMEOUT      V6
#define VPIN_B1_MOTION_TIMEOUT_NUM  6

void initDashboard();
void updateDashboard();
void notifyDeviceStateChange(
    uint8_t deviceID,
    bool newState
);
void updateAllBlynkWidgets();
void setDeviceMode(uint8_t deviceID, uint8_t newMode);