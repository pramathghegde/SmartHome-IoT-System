#include <WiFi.h>
#include <esp_now.h>

#include "espnow_manager.h"

#include "packet.h"

#include "commands.h"

#include "node_ids.h"

#include "mac_addresses.h"

#include "state_manager.h"

#include "node_manager.h"

static Packet rxPacket;

void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    memcpy(
        &rxPacket,
        incomingData,
        sizeof(rxPacket)
    );

    switch(rxPacket.command)
    {
        case CMD_HEARTBEAT:

            updateHeartbeat(
                rxPacket.senderNode
            );

            break;

        case CMD_MOTION:

            if(
                rxPacket.senderNode ==
                BEDROOM1_NODE
            )
            {
                bedroom1.motionDetected =
                    rxPacket.motionDetected;
            }

            break;

        case CMD_ENVIRONMENT:

            if(
                rxPacket.senderNode ==
                BEDROOM1_NODE
            )
            {
                bedroom1.brightness =
                    rxPacket.brightness;
            }

            break;
    }
}

void initEspNow()
{
    WiFi.mode(WIFI_STA);

    if(
        esp_now_init()
        != ESP_OK
    )
    {
        Serial.println(
            "ESP-NOW Init Failed"
        );

        return;
    }

    esp_now_register_recv_cb(
        onDataRecv
    );

    esp_now_peer_info_t peerInfo = {};

    memcpy(
        peerInfo.peer_addr,
        BEDROOM1_MAC,
        6
    );

    peerInfo.channel = 0;

    peerInfo.encrypt = false;

    esp_now_add_peer(
        &peerInfo
    );

    Serial.println(
        "ESP-NOW Ready"
    );
}

void sendDeviceCommand(
    uint8_t targetNode,
    uint8_t deviceID,
    bool state
)
{
    Packet tx;

    tx.senderNode =
        MASTER_NODE;

    tx.receiverNode =
        targetNode;

    tx.command =
        CMD_SET_DEVICE_STATE;

    tx.deviceID =
        deviceID;

    tx.state =
        state;

    if(
        targetNode ==
        BEDROOM1_NODE
    )
    {
        esp_now_send(
            BEDROOM1_MAC,
            (uint8_t*)&tx,
            sizeof(tx)
        );
    }
}

void processIncomingPackets()
{
}