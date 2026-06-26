#pragma once

void initMotionSensor();

void updateMotionSensor();

// Returns raw RCWL GPIO state only.
// No latch, no timeout, no decision logic.
// Master receives this raw value and applies all timeout/decision logic.
bool isMotionDetected();

bool hasMotionChanged();

unsigned long getLastMotionTime();
