#pragma once

#include "device.h"

void initDevices();

Device* getDevice(uint8_t deviceID);

void setDeviceState(uint8_t deviceID, bool state);

bool getDeviceState(uint8_t deviceID);

void setDeviceMode(uint8_t deviceID, uint8_t mode);

uint8_t getDeviceMode(uint8_t deviceID);