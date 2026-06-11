#include <Arduino.h>

static unsigned long bedroom1LastSeen = 0;

void updateNodeHeartbeat(
    uint8_t nodeID
)
{
    if(nodeID == 2)
    {
        bedroom1LastSeen =
            millis();
    }
}

bool isNodeOnline(
    uint8_t nodeID
)
{
    if(nodeID == 2)
    {
        return
        (
            millis()
            -
            bedroom1LastSeen
        )
        <
        90000;
    }

    return false;
}