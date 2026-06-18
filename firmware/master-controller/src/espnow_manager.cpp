#include <WiFi.h>
#include <esp_now.h>

#include "espnow_manager.h"
#include "packet.h"
#include "commands.h"
#include "node_ids.h"
#include "mac_addresses.h"
#include "state_manager.h"
#include "node_manager.h"

#include <Arduino.h>

static Packet rxPacket;
static volatile bool lastSendSuccess = false;

void onDataSent(
    const uint8_t *mac_addr,
    esp_now_send_status_t status
)
{
    lastSendSuccess = (status == ESP_NOW_SEND_SUCCESS);
}

bool getLastSendSuccess()
{
    return lastSendSuccess;
}

void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    memcpy(&rxPacket, incomingData, sizeof(rxPacket));

    switch(rxPacket.command)
    {
        case CMD_HEARTBEAT:

            Serial.print("[HEARTBEAT] Node=");
            Serial.print(rxPacket.senderNode);
            Serial.print(" Motion=");
            Serial.print(rxPacket.motionDetected);
            Serial.print(" Bright=");
            Serial.print(rxPacket.brightness);
            Serial.print(" Uptime=");
            Serial.print(rxPacket.uptime / 1000);
            Serial.println("s");

            updateHeartbeat(rxPacket.senderNode);

            if (rxPacket.senderNode == BEDROOM1_NODE)
            {
                bedroom1.motionDetected =
                    rxPacket.motionDetected;

                bedroom1.brightness =
                    rxPacket.brightness;

                sendAck(BEDROOM1_NODE);
            }

            break;

        case CMD_MOTION:

            if (rxPacket.senderNode == BEDROOM1_NODE)
            {
                bedroom1.motionDetected =
                    rxPacket.motionDetected;
            }

            break;

        case CMD_ENVIRONMENT:

            if (rxPacket.senderNode == BEDROOM1_NODE)
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

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("[ESP-NOW] Init Failed");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};

    memcpy(
        peerInfo.peer_addr,
        BEDROOM1_MAC,
        6
    );

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);

    Serial.println("[ESP-NOW] Ready");
}

void sendAck(uint8_t targetNode)
{
    Packet tx = {};

    tx.senderNode   = MASTER_NODE;
    tx.receiverNode = targetNode;
    tx.command      = CMD_ACK;
    tx.uptime       = millis();

    if (targetNode == BEDROOM1_NODE)
    {
        esp_now_send(
            BEDROOM1_MAC,
            (uint8_t*)&tx,
            sizeof(tx)
        );
    }
}

void sendDeviceCommand(
    uint8_t targetNode,
    uint8_t deviceID,
    bool state
)
{
    Packet tx = {};

    tx.senderNode   = MASTER_NODE;
    tx.receiverNode = targetNode;
    tx.command      = CMD_SET_DEVICE_STATE;
    tx.deviceID     = deviceID;
    tx.state        = state;
    tx.uptime       = millis();

    lastSendSuccess = false;

    if (targetNode == BEDROOM1_NODE)
    {
        esp_now_send(
            BEDROOM1_MAC,
            (uint8_t*)&tx,
            sizeof(tx)
        );

        // Wait for send callback - max 50ms
        unsigned long wait = millis();
        while (
            !lastSendSuccess &&
            (millis() - wait) < 50
        )
        {
            delay(1);
        }

        Serial.print("[SEND] Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.print(state ? "ON" : "OFF");
        Serial.println(
            lastSendSuccess ? " OK" : " FAIL"
        );
    }
}
