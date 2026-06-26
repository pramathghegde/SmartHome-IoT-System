#pragma once

#include <stdint.h>

struct Packet
{
    uint8_t senderNode;

    uint8_t receiverNode;

    uint8_t command;

    uint8_t deviceID;

    uint8_t state;

    uint8_t mode;

    uint8_t fanSpeed;

    bool motionDetected;

    int brightness;

    uint32_t uptime;

    uint32_t bootCount;
};

static_assert(sizeof(Packet) == 20, "Packet size mismatch; master and nodes must use the same packet layout");
