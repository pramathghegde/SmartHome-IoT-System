#pragma once

void initMotionSensor();

void updateMotionSensor();

bool isMotionDetected();

bool hasMotionChanged();

unsigned long getLastMotionTime();