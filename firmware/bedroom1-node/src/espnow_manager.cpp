#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <WiFi.h>

#include "espnow_manager.h"
#include "packet.h"
#include "commands.h"
#include "config.h"
#include "node_ids.h"
#include "mac_addresses.h"
#include "device_manager.h"
#include "relay_manager.h"
#include "motion_manager.h"
#include "environment_manager.h"

#include <Arduino.h>

static unsigned long lastHeartbeat       = 0;
static QueueHandle_t rxQueue             = nullptr;
static QueueHandle_t relayCommandQueue   = nullptr;
static volatile uint32_t rxQueueOverflow = 0;
static volatile uint32_t relayQueueOverflow = 0;
static volatile uint32_t rxQueueMaxUsed  = 0;
static volatile uint32_t relayQueueMaxUsed = 0;
static volatile uint32_t rxPacketsQueued = 0;
static volatile uint32_t relayPacketsQueued = 0;
static volatile uint32_t rxCallbacks     = 0;

extern int bootCount;

static bool isValidCommand(uint8_t command)
{
    switch (command)
    {
        case CMD_ACK:
        case CMD_SET_DEVICE_STATE:
        case CMD_SET_MODE:
        case CMD_FAN_SPEED:
            return true;

        default:
            return false;
    }
}

// ESP-NOW receive callback - runs in ISR context, must be fast
void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    if (len != sizeof(Packet))
    {
        return;
    }

    Packet packet;
    memcpy(&packet, incomingData, sizeof(packet));

    if (packet.receiverNode != NODE_ID)
    {
        return;
    }

    if (!isValidCommand(packet.command))
    {
        return;
    }

    rxCallbacks++;

    if (rxQueue != nullptr)
    {
        QueueHandle_t targetQueue =
            (packet.command == CMD_SET_DEVICE_STATE) ? relayCommandQueue : rxQueue;

        if (targetQueue == nullptr)
        {
            if (packet.command == CMD_SET_DEVICE_STATE)
            {
                relayQueueOverflow++;
            }
            else
            {
                rxQueueOverflow++;
            }
            return;
        }

        if (xQueueSend(targetQueue, &packet, 0) != pdTRUE)
        {
            if (packet.command == CMD_SET_DEVICE_STATE)
            {
                relayQueueOverflow++;
            }
            else
            {
                rxQueueOverflow++;
            }
        }
        else
        {
            UBaseType_t used = uxQueueMessagesWaiting(targetQueue);

            if (packet.command == CMD_SET_DEVICE_STATE)
            {
                relayPacketsQueued++;

                if (used > relayQueueMaxUsed)
                {
                    relayQueueMaxUsed = used;
                }
            }
            else
            {
                rxPacketsQueued++;

                if (used > rxQueueMaxUsed)
                {
                    rxQueueMaxUsed = used;
                }
            }
        }
    }
}

void initEspNow()
{
    // NOTE: WiFi.mode(WIFI_STA) is NOT called here.
    // It is called once in initOTA() before esp_now_init().
    // Calling WiFi.mode() a second time after ESP-NOW init
    // can disrupt the ESP-NOW stack. Do not add it back.

    relayCommandQueue = xQueueCreate(10, sizeof(Packet));
    rxQueue = xQueueCreate(20, sizeof(Packet));

    if (rxQueue == nullptr || relayCommandQueue == nullptr)
    {
        Serial.println("[ESP-NOW] Failed to create RX queues");
        return;
    }

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

    Packet txPacket = {};

    txPacket.senderNode     = NODE_ID;
    txPacket.receiverNode   = MASTER_NODE;
    txPacket.command        = CMD_HEARTBEAT;
    txPacket.motionDetected = isMotionDetected();
    txPacket.brightness     = environment.brightness;
    txPacket.uptime         = millis() / 1000; // seconds, overflow safe
    txPacket.bootCount      = bootCount;

    // Pack diagnostics into unused fields of the heartbeat packet
    txPacket.deviceID       = (uint8_t)esp_reset_reason();
    txPacket.state          = (uint8_t)WiFi.status();
    
    uint32_t minHeapKb      = ESP.getMinFreeHeap() / 1024;
    txPacket.mode           = (minHeapKb > 255) ? 255 : (uint8_t)minHeapKb;
    
    uint32_t stackHighWater = uxTaskGetStackHighWaterMark(nullptr) / 32;
    txPacket.fanSpeed       = (stackHighWater > 255) ? 255 : (uint8_t)stackHighWater;

    esp_err_t result = esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );

    Serial.print("[HB] Motion=");
    Serial.print(txPacket.motionDetected);
    Serial.print(" Bright=");
    Serial.print(txPacket.brightness);
    Serial.print(" Uptime=");
    Serial.print(txPacket.uptime);
    Serial.print("s Boot=");
    Serial.print(txPacket.bootCount);
    Serial.println(result == ESP_OK ? " OK" : " FAIL");
}

void sendMotionStatus(bool motion)
{
    Packet txPacket = {};

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

static void sendCommandAck(uint8_t deviceID, bool state)
{
    Packet txPacket = {};

    txPacket.senderNode   = NODE_ID;
    txPacket.receiverNode = MASTER_NODE;
    txPacket.command      = CMD_ACK;
    txPacket.deviceID     = deviceID;
    txPacket.state        = state;
    txPacket.uptime       = millis();
    txPacket.bootCount    = bootCount;

    esp_err_t result = esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );

    Serial.print("[ESP SEND] CMD_ACK Device=");
    Serial.print(deviceID);
    Serial.print(" State=");
    Serial.print(state ? "ON" : "OFF");
    Serial.println(result == ESP_OK ? " queued" : " queue-failed");
}

void sendEnvironmentStatus()
{
    Packet txPacket = {};

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
    if (rxQueue == nullptr || relayCommandQueue == nullptr)
    {
        return;
    }

    // Throttled overflow logging
    if (rxQueueOverflow > 0)
    {
        static unsigned long lastOverflowLog = 0;

        if (millis() - lastOverflowLog > 5000)
        {
            Serial.print("[ESP-NOW] Dropped control packets: ");
            Serial.println(rxQueueOverflow);
            rxQueueOverflow  = 0;
            lastOverflowLog  = millis();
        }
    }

    if (relayQueueOverflow > 0)
    {
        static unsigned long lastRelayOverflowLog = 0;

        if (millis() - lastRelayOverflowLog > 1000)
        {
            Serial.print("[ESP-NOW] Dropped relay packets: ");
            Serial.println(relayQueueOverflow);
            relayQueueOverflow  = 0;
            lastRelayOverflowLog  = millis();
        }
    }

    Packet packet;

    while (xQueueReceive(relayCommandQueue, &packet, 0) == pdTRUE)
    {
        Serial.print("[RX QUEUE POP] Relay cmd=");
        Serial.print(packet.command);
        Serial.print(" waiting=");
        Serial.println(uxQueueMessagesWaiting(relayCommandQueue));

        Serial.print("[COMMAND EXECUTED] Device=");
        Serial.print(packet.deviceID);
        Serial.print(" State=");
        Serial.println(packet.state ? "ON" : "OFF");

        applyRelayCommand(packet.deviceID, packet.state);
        sendCommandAck(packet.deviceID, packet.state);

        Serial.print("[CMD] Device=");
        Serial.print(packet.deviceID);
        Serial.print(" -> ");
        Serial.println(packet.state ? "ON" : "OFF");
    }

    while (xQueueReceive(rxQueue, &packet, 0) == pdTRUE)
    {
        Serial.print("[RX QUEUE POP] Cmd=");
        Serial.print(packet.command);
        Serial.print(" waiting=");
        Serial.println(uxQueueMessagesWaiting(rxQueue));

        switch(packet.command)
        {
            case CMD_ACK:
                // Silent. ACK just confirms master is alive.
                // Bedroom1 takes no action based on master presence.
                break;

            case CMD_SET_MODE:
                setDeviceMode(packet.deviceID, packet.mode);

                Serial.print("[CMD] Mode Device=");
                Serial.print(packet.deviceID);
                Serial.print(" -> ");
                Serial.println(packet.mode);
                break;

            case CMD_FAN_SPEED:
                // Reserved for future fan speed control
                break;

            default:
                break;
        }
    }
}

void printEspNowDiagnostics()
{
    if (rxQueue == nullptr || relayCommandQueue == nullptr)
    {
        Serial.println("[QUEUE] ESP-NOW RX queues not created");
        return;
    }

    Serial.print("[QUEUE] ESP-NOW RX free=");
    Serial.print(uxQueueSpacesAvailable(rxQueue));
    Serial.print("/20 dropped=");
    Serial.print(rxQueueOverflow);
    Serial.print(" maxUsed=");
    Serial.print(rxQueueMaxUsed);
    Serial.print(" queued=");
    Serial.print(rxPacketsQueued);
    Serial.print(" callbacks=");
    Serial.println(rxCallbacks);
    Serial.print("[QUEUE] RELAY RX free=");
    Serial.print(uxQueueSpacesAvailable(relayCommandQueue));
    Serial.print("/10 dropped=");
    Serial.print(relayQueueOverflow);
    Serial.print(" maxUsed=");
    Serial.print(relayQueueMaxUsed);
    Serial.print(" queued=");
    Serial.println(relayPacketsQueued);
}
