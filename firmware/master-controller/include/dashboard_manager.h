#pragma once

void initDashboard();
void updateDashboard();
void notifyDeviceStateChange(
    uint8_t deviceID,
    bool newState
);