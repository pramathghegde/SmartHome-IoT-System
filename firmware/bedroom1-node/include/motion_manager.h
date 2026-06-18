#pragma once

void initMotionSensor();

void updateMotionSensor();

bool isMotionDetected();

void markMotionReported();

bool hasMotionChanged();

unsigned long getLastMotionTime();
