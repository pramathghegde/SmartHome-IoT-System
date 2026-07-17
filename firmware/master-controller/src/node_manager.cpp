#include <Arduino.h>

#include "node_manager.h"

#include "state_manager.h"

#include "config.h"

#include "node_ids.h"

static bool b1OfflinePrinted = false;
static bool lrOfflinePrinted = false;

const char* getNodeName(uint8_t nodeID)
{
    switch (nodeID)
    {
        case MASTER_NODE:     return "MASTER";
        case BEDROOM1_NODE:   return "BEDROOM1";
        case BEDROOM2_NODE:   return "BEDROOM2";
        case HALL_NODE:       return "HALL";
        case KITCHEN_NODE:    return "KITCHEN";
        case OUTDOOR_NODE:    return "OUTDOOR";
        case TANK_NODE:       return "TANK";
        case DOORLOCK_NODE:   return "DOORLOCK";
        case LIVINGROOM_NODE: return "LIVINGROOM";
        default:              return "UNKNOWN";
    }
}

void updateHeartbeat(uint8_t nodeID)
{
    if(nodeID == BEDROOM1_NODE)
    {
        if(!bedroom1.online)
        {
            Serial.print("[NODE] ");
            Serial.print(getNodeName(nodeID));
            Serial.println(" ONLINE");
            bedroom1.syncPending = true;
        }

        bedroom1.online = true;
        bedroom1.lastHeartbeat = millis();
        b1OfflinePrinted = false;
    }
    else if(nodeID == LIVINGROOM_NODE)
    {
        if(!livingroom.online)
        {
            Serial.print("[NODE] ");
            Serial.print(getNodeName(nodeID));
            Serial.println(" ONLINE");
            livingroom.syncPending = true;
        }

        livingroom.online = true;
        livingroom.lastHeartbeat = millis();
        lrOfflinePrinted = false;
    }
}

void checkNodeStatus()
{
    // Bedroom1 status check
    if (millis() - bedroom1.lastHeartbeat > HEARTBEAT_TIMEOUT)
    {
        bedroom1.online = false;

        if(!b1OfflinePrinted)
        {
            Serial.print("[NODE] ");
            Serial.print(getNodeName(BEDROOM1_NODE));
            Serial.println(" OFFLINE");

            b1OfflinePrinted = true;
        }
    }

    // LivingRoom status check
    if (millis() - livingroom.lastHeartbeat > HEARTBEAT_TIMEOUT)
    {
        livingroom.online = false;

        if(!lrOfflinePrinted)
        {
            Serial.print("[NODE] ");
            Serial.print(getNodeName(LIVINGROOM_NODE));
            Serial.println(" OFFLINE");

            lrOfflinePrinted = true;
        }
    }
}