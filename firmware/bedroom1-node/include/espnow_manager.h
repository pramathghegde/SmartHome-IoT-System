#pragma once

void initEspNow();

void sendHeartbeat();

void sendMotionStatus(bool motion);

void sendEnvironmentStatus();

void processIncomingPackets();

void printEspNowDiagnostics();

// isMasterOnline() REMOVED.
// Bedroom1 must never change behaviour based on master presence.
// Bedroom1 simply keeps last relay states and waits.
