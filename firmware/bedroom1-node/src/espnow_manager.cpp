#include <WiFi.h>
#include <esp_now.h>

#include "espnow_manager.h"
#include "packet.h"
#include "commands.h"
#include "config.h"
#include "node_ids.h"
#include "mac_addresses.h"
#include "device_manager.h"
#include "motion_manager.h"
#include "environment_manager.h"

#include <Arduino.h>

static Packet txPacket;
static unsigned long lastHeartbeat   = 0;
static unsigned long lastMasterPacket = 0;

bool isMasterOnline()
{
    return (millis() - lastMasterPacket) < MASTER_TIMEOUT;
}

void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    Packet packet;

    memcpy(&packet, incomingData, sizeof(packet));

    lastMasterPacket = millis();

    switch(packet.command)
    {
        case CMD_ACK:
            // Silent - no serial print needed, too noisy
            break;

        case CMD_SET_DEVICE_STATE:

            setDeviceState(packet.deviceID, packet.state);

            Serial.print("[CMD] Device=");
            Serial.print(packet.deviceID);
            Serial.print(" -> ");
            Serial.println(packet.state ? "ON" : "OFF");

            break;

        case CMD_SET_MODE:

            setDeviceMode(packet.deviceID, packet.mode);

            Serial.print("[CMD] Mode Device=");
            Serial.print(packet.deviceID);
            Serial.print(" -> ");
            Serial.println(packet.mode);

            break;

        case CMD_FAN_SPEED:
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

    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};

    memcpy(peerInfo.peer_addr, MASTER_MAC, 6);

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);

    Serial.println("[ESP-NOW] Ready");
}

void sendHeartbeat()
{
    if (millis() - lastHeartbeat < HEARTBEAT_INTERVAL)
    {
        return;
    }

    lastHeartbeat = millis();

    txPacket = {};

    txPacket.senderNode     = NODE_ID;
    txPacket.receiverNode   = MASTER_NODE;
    txPacket.command        = CMD_HEARTBEAT;
    txPacket.motionDetected = isMotionDetected();
    txPacket.brightness     = environment.brightness;

    // Uptime in seconds, overflow safe
    txPacket.uptime = millis() / 1000;

    esp_err_t result = esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );

    if (txPacket.motionDetected && result == ESP_OK)
    {
        markMotionReported();
    }

    Serial.print("[HB] Motion=");
    Serial.print(txPacket.motionDetected);
    Serial.print(" Bright=");
    Serial.print(txPacket.brightness);
    Serial.print(" Uptime=");
    Serial.print(txPacket.uptime);
    Serial.println(
        result == ESP_OK ? "s OK" : "s FAIL"
    );
}

void sendMotionStatus(bool motion)
{
    txPacket = {};

    txPacket.senderNode     = NODE_ID;
    txPacket.receiverNode   = MASTER_NODE;
    txPacket.command        = CMD_MOTION;
    txPacket.motionDetected = motion;

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );
}

void sendEnvironmentStatus()
{
    txPacket = {};

    txPacket.senderNode   = NODE_ID;
    txPacket.receiverNode = MASTER_NODE;
    txPacket.command      = CMD_ENVIRONMENT;
    txPacket.brightness   = environment.brightness;

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );
}

void processIncomingPackets()
{
    // ESP-NOW is interrupt driven via callback
    // Nothing needed here
}
