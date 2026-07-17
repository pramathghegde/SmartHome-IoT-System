#pragma once

struct EnvironmentState
{
    float temperature;
    float humidity;
};

extern EnvironmentState environment;

void initEnvironment();
void updateEnvironment();