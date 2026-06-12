#include "automation_manager.h"

#include "state_manager.h"

#include "espnow_manager.h"

#include "device_ids.h"

#include "node_ids.h"

#include "schedule_manager.h"

#include "device_cache.h"

void runAutomation()
{
    if(!bedroom1.online)
    {
        return;
    }

    bool desiredLightState = false;

    if(!isNightRestrictionActive())
    {
        if(
            bedroom1.motionDetected
            &&
            bedroom1.brightness < 1200
        )
        {
            desiredLightState = true;
        }
    }

    if(
        getTubeLightState()
        !=
        desiredLightState
    )
    {
        sendDeviceCommand(
            BEDROOM1_NODE,
            TUBELIGHT_DEVICE,
            desiredLightState
        );

        setTubeLightState(
            desiredLightState
        );
    }

    if(
        getBulbState()
        !=
        desiredLightState
    )
    {
        sendDeviceCommand(
            BEDROOM1_NODE,
            BULB_DEVICE,
            desiredLightState
        );

        setBulbState(
            desiredLightState
        );
    }
}