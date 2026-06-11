#include <stdint.h>

struct Packet
{
    uint8_t senderNode;

    uint8_t receiverNode;

    uint8_t command;

    uint8_t deviceID;

    uint8_t state;

    uint8_t mode;

    bool motionDetected;

    uint32_t uptime;
};