#pragma once

void initEspNow();

void sendHeartbeat();

void sendEnvironmentStatus();

void sendMotionStatus(bool motion);

void processIncomingPackets();

bool isMasterOnline();