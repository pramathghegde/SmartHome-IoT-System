#include "environment_manager.h"

#include "ldr_manager.h"

EnvironmentState environment =
{
    0
};

void updateEnvironment()
{
    environment.brightness =
        getBrightness();
}