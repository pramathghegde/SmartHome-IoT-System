#pragma once

void initEspNow();

void sendHeartbeat();

void sendMotionStatus(bool motion);

void processIncomingPackets();

void printEspNowDiagnostics();

// isMasterOnline() REMOVED.
// DiningHall must never change behaviour based on master presence.
// DiningHall simply keeps last relay states and waits.
