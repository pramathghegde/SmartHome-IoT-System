#include <Arduino.h>

#include "node_manager.h"

#include "state_manager.h"

#include "config.h"

#include "node_ids.h"

void updateHeartbeat(uint8_t nodeID)
{
    if(nodeID == BEDROOM1_NODE)
    {
        bedroom1.online = true;

        bedroom1.lastHeartbeat =
            millis();
    }
}

void checkNodeStatus()
{
    if(
        millis()
        -
        bedroom1.lastHeartbeat
        >
        HEARTBEAT_TIMEOUT
    )
    {
        bedroom1.online = false;
    }
}