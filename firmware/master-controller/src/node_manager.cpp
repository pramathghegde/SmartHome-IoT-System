#include <Arduino.h>

#include "node_manager.h"

#include "state_manager.h"

#include "config.h"

#include "node_ids.h"

void updateHeartbeat(uint8_t nodeID)
{
    Serial.println(
        "[NODE] BEDROOM1 ONLINE"
    );
    if(nodeID == BEDROOM1_NODE)
    {
        bedroom1.online = true;

        bedroom1.lastHeartbeat =
            millis();
    }
}

void checkNodeStatus()
{
    Serial.println(
        "[NODE] BEDROOM1 OFFLINE"
    );
    if(
        millis()
        -
        bedroom1.lastHeartbeat
        >
        HEARTBEAT_TIMEOUT
    )
    {
        bedroom1.online = false;

        bedroom1.motionDetected = false;

        bedroom1.brightness = 0;
    }
}