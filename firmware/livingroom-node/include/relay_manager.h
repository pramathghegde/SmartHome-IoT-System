#pragma once

#include <stdint.h>

void initRelays();

void applyRelayCommand(uint8_t deviceID, bool state);
