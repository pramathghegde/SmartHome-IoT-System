#pragma once

struct EnvironmentState
{
    int brightness;
};

extern EnvironmentState environment;

void updateEnvironment();