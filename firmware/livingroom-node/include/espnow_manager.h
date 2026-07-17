#pragma once

void initEspNow();

void sendHeartbeat();

void sendMotionStatus(bool motion);

void processIncomingPackets();

void printEspNowDiagnostics();

// isMasterOnline() REMOVED.
// LivingRoom must never change behaviour based on master presence.
// LivingRoom simply keeps last relay states and waits.
