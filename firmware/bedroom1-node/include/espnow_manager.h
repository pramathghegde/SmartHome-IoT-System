#pragma once

void initEspNow();

void sendHeartbeat();

void sendMotionStatus(bool motion);

void sendEnvironmentStatus();

void processIncomingPackets();

bool isMasterOnline();