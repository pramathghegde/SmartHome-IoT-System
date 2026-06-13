#include <Arduino.h>

#include "node_manager.h"

#include "state_manager.h"

#include "config.h"

#include "node_ids.h"

static bool offlinePrinted = false;

void updateHeartbeat(uint8_t nodeID)
{
    if(nodeID == BEDROOM1_NODE)
    {
        if(!bedroom1.online)
        {
            Serial.println(
                "[NODE] BEDROOM1 ONLINE"
            );
        }

        bedroom1.online = true;

        bedroom1.lastHeartbeat =
            millis();

        offlinePrinted = false;
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

        if(!offlinePrinted)
        {
            Serial.println(
                "[NODE] BEDROOM1 OFFLINE"
            );

            offlinePrinted = true;
        }
    }
}