#pragma once

#include <stdint.h>

void runAutomation();

void recordMotionEvent(uint8_t nodeID);

void confirmDeviceCommand(uint8_t nodeID, uint8_t deviceID, bool state);

