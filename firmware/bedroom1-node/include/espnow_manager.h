#pragma once

void initEspNow();

void sendHeartbeat();

void sendMotionStatus(bool motion);

void processIncomingPackets();