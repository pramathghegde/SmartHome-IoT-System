#pragma once

#include <stdint.h>

void initEspNow();

bool sendDeviceCommand(
    uint8_t targetNode,
    uint8_t deviceID,
    bool state
);

void sendAck(uint8_t targetNode);

bool getLastSendSuccess();

void processIncomingPackets();

void printEspNowDiagnostics();
void getEspNowQueueStats(uint32_t &maxUsed, uint32_t &overflow);

