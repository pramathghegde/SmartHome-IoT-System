#include "device_cache.h"

#include "modes.h"

DeviceConfig bedroom1Fan;
DeviceConfig bedroom1Tube;
DeviceConfig bedroom1Bulb;
DeviceConfig bedroom1Socket;
DeviceConfig bedroom1AC;
bool bedroom1LdrEnabled = true;

static void putUCharIfChanged(Preferences &prefs, const char* key, uint8_t val)
{
    if (!prefs.isKey(key) || prefs.getUChar(key) != val)
    {
        prefs.putUChar(key, val);
        Serial.printf("[NVS] Write %s -> %u\n", key, val);
    }
}

bool validateConfiguration(DeviceConfig &device, const DeviceConfig &defaultConfig)
{
    bool valid = true;

    if (device.mode > 3)
    {
        device.mode = defaultConfig.mode;
        valid = false;
    }

    // Validate AUTO schedule
    if (device.autoStartHour > 23)
    {
        device.autoStartHour = defaultConfig.autoStartHour;
        valid = false;
    }
    if (device.autoStartMinute > 59)
    {
        device.autoStartMinute = defaultConfig.autoStartMinute;
        valid = false;
    }
    if (device.autoStopHour > 23)
    {
        device.autoStopHour = defaultConfig.autoStopHour;
        valid = false;
    }
    if (device.autoStopMinute > 59)
    {
        device.autoStopMinute = defaultConfig.autoStopMinute;
        valid = false;
    }

    // Validate SCHEDULE schedule
    if (device.schedStartHour > 23)
    {
        device.schedStartHour = defaultConfig.schedStartHour;
        valid = false;
    }
    if (device.schedStartMinute > 59)
    {
        device.schedStartMinute = defaultConfig.schedStartMinute;
        valid = false;
    }
    if (device.schedStopHour > 23)
    {
        device.schedStopHour = defaultConfig.schedStopHour;
        valid = false;
    }
    if (device.schedStopMinute > 59)
    {
        device.schedStopMinute = defaultConfig.schedStopMinute;
        valid = false;
    }

    return valid;
}

void saveDeviceConfiguration(Preferences &prefs, const char* prefix, const DeviceConfig &device)
{
    char key[16];

    snprintf(key, sizeof(key), "%s_mode", prefix);
    putUCharIfChanged(prefs, key, device.mode);

    // Save AUTO schedule
    snprintf(key, sizeof(key), "%s_a_st_h", prefix);
    putUCharIfChanged(prefs, key, device.autoStartHour);

    snprintf(key, sizeof(key), "%s_a_st_m", prefix);
    putUCharIfChanged(prefs, key, device.autoStartMinute);

    snprintf(key, sizeof(key), "%s_a_sp_h", prefix);
    putUCharIfChanged(prefs, key, device.autoStopHour);

    snprintf(key, sizeof(key), "%s_a_sp_m", prefix);
    putUCharIfChanged(prefs, key, device.autoStopMinute);

    // Save SCHEDULE schedule
    snprintf(key, sizeof(key), "%s_s_st_h", prefix);
    putUCharIfChanged(prefs, key, device.schedStartHour);

    snprintf(key, sizeof(key), "%s_s_st_m", prefix);
    putUCharIfChanged(prefs, key, device.schedStartMinute);

    snprintf(key, sizeof(key), "%s_s_sp_h", prefix);
    putUCharIfChanged(prefs, key, device.schedStopHour);

    snprintf(key, sizeof(key), "%s_s_sp_m", prefix);
    putUCharIfChanged(prefs, key, device.schedStopMinute);
}

bool loadDeviceConfiguration(Preferences &prefs, const char* prefix, DeviceConfig &device, const DeviceConfig &defaultConfig)
{
    char key[16];
    bool keysMissing = false;

    snprintf(key, sizeof(key), "%s_mode", prefix);
    if (prefs.isKey(key)) device.mode = prefs.getUChar(key);
    else { device.mode = defaultConfig.mode; keysMissing = true; }

    // Load AUTO schedule
    snprintf(key, sizeof(key), "%s_a_st_h", prefix);
    if (prefs.isKey(key)) device.autoStartHour = prefs.getUChar(key);
    else { device.autoStartHour = defaultConfig.autoStartHour; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_a_st_m", prefix);
    if (prefs.isKey(key)) device.autoStartMinute = prefs.getUChar(key);
    else { device.autoStartMinute = defaultConfig.autoStartMinute; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_a_sp_h", prefix);
    if (prefs.isKey(key)) device.autoStopHour = prefs.getUChar(key);
    else { device.autoStopHour = defaultConfig.autoStopHour; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_a_sp_m", prefix);
    if (prefs.isKey(key)) device.autoStopMinute = prefs.getUChar(key);
    else { device.autoStopMinute = defaultConfig.autoStopMinute; keysMissing = true; }

    // Load SCHEDULE schedule
    snprintf(key, sizeof(key), "%s_s_st_h", prefix);
    if (prefs.isKey(key)) device.schedStartHour = prefs.getUChar(key);
    else { device.schedStartHour = defaultConfig.schedStartHour; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_s_st_m", prefix);
    if (prefs.isKey(key)) device.schedStartMinute = prefs.getUChar(key);
    else { device.schedStartMinute = defaultConfig.schedStartMinute; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_s_sp_h", prefix);
    if (prefs.isKey(key)) device.schedStopHour = prefs.getUChar(key);
    else { device.schedStopHour = defaultConfig.schedStopHour; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_s_sp_m", prefix);
    if (prefs.isKey(key)) device.schedStopMinute = prefs.getUChar(key);
    else { device.schedStopMinute = defaultConfig.schedStopMinute; keysMissing = true; }

    device.currentState = defaultConfig.currentState; // DO NOT load currentState

    bool valid = validateConfiguration(device, defaultConfig);
    return (valid && !keysMissing);
}

void saveConfiguration()
{
    Preferences prefs;
    prefs.begin("automation", false);
    saveDeviceConfiguration(prefs, "fan",  bedroom1Fan);
    saveDeviceConfiguration(prefs, "tube", bedroom1Tube);
    saveDeviceConfiguration(prefs, "bulb", bedroom1Bulb);
    saveDeviceConfiguration(prefs, "sock", bedroom1Socket);
    saveDeviceConfiguration(prefs, "ac",   bedroom1AC);
    prefs.end();
}

void saveSingleDevice(const char* prefix, const DeviceConfig &device)
{
    Preferences prefs;
    prefs.begin("automation", false);
    saveDeviceConfiguration(prefs, prefix, device);
    prefs.end();
}

void saveRoomLdrEnabled(bool enabled)
{
    Preferences prefs;
    prefs.begin("automation", false);
    putUCharIfChanged(prefs, "b1_ldr_en", enabled ? 1 : 0);
    prefs.end();
}

void loadConfiguration()
{
    Preferences prefs;
    prefs.begin("automation", false);

    // Safe Defaults
    DeviceConfig defaultFan    = { 2, false, 23, 0, 5, 0,  23, 0, 5, 0 };  // MODE_AUTO = 2
    DeviceConfig defaultTube   = { 2, false, 18, 0, 23, 0, 18, 0, 23, 0 };
    DeviceConfig defaultBulb   = { 2, false, 18, 0, 23, 0, 18, 0, 23, 0 };
    DeviceConfig defaultSocket = { 0, false, 0, 0, 0, 0,  0, 0, 0, 0 };   // MODE_OFF = 0
    DeviceConfig defaultAC     = { 0, false, 0, 0, 0, 0,  0, 0, 0, 0 };

    bool fanOk  = loadDeviceConfiguration(prefs, "fan",  bedroom1Fan,    defaultFan);
    bool tubeOk = loadDeviceConfiguration(prefs, "tube", bedroom1Tube,   defaultTube);
    bool bulbOk = loadDeviceConfiguration(prefs, "bulb", bedroom1Bulb,   defaultBulb);
    bool sockOk = loadDeviceConfiguration(prefs, "sock", bedroom1Socket, defaultSocket);
    bool acOk   = loadDeviceConfiguration(prefs, "ac",   bedroom1AC,     defaultAC);

    if (!fanOk)  saveDeviceConfiguration(prefs, "fan",  bedroom1Fan);
    if (!tubeOk) saveDeviceConfiguration(prefs, "tube", bedroom1Tube);
    if (!bulbOk) saveDeviceConfiguration(prefs, "bulb", bedroom1Bulb);
    if (!sockOk) saveDeviceConfiguration(prefs, "sock", bedroom1Socket);
    if (!acOk)   saveDeviceConfiguration(prefs, "ac",   bedroom1AC);

    // Load LDR Enable configuration
    bool ldrVal = true;
    bool needsWrite = false;
    if (prefs.isKey("b1_ldr_en"))
    {
        uint8_t val = prefs.getUChar("b1_ldr_en");
        if (val == 0 || val == 1)
        {
            ldrVal = (val == 1);
        }
        else
        {
            ldrVal = true;
            needsWrite = true;
        }
    }
    else
    {
        ldrVal = true;
        needsWrite = true;
    }
    bedroom1LdrEnabled = ldrVal;
    if (needsWrite)
    {
        putUCharIfChanged(prefs, "b1_ldr_en", 1);
    }

    prefs.end();
}

static const char* getModeName(uint8_t mode)
{
    switch (mode)
    {
        case 0: return "OFF";
        case 1: return "ON";
        case 2: return "AUTO";
        case 3: return "SCHEDULE";
        default: return "UNKNOWN";
    }
}

void printRestoredConfiguration()
{
    Serial.println("=================================");
    Serial.println("RESTORED CONFIGURATION");
    Serial.printf("Fan      : %s\n", getModeName(bedroom1Fan.mode));
    Serial.printf("Tube     : %s\n", getModeName(bedroom1Tube.mode));
    Serial.printf("Bulb     : %s\n", getModeName(bedroom1Bulb.mode));
    Serial.printf("Socket   : %s\n", getModeName(bedroom1Socket.mode));
    Serial.printf("AC       : %s\n", getModeName(bedroom1AC.mode));
    Serial.printf("LDR Enable: %s\n", bedroom1LdrEnabled ? "ON" : "OFF");
    Serial.println("Schedules (AUTO / SCHEDULE):");

    auto printDualSchedule = [](const char* name, const DeviceConfig &device) {
        Serial.printf("%-9sAUTO: %02u:%02u - %02u:%02u  |  SCHED: %02u:%02u - %02u:%02u\n",
                      name,
                      device.autoStartHour, device.autoStartMinute, device.autoStopHour, device.autoStopMinute,
                      device.schedStartHour, device.schedStartMinute, device.schedStopHour, device.schedStopMinute);
    };

    printDualSchedule("Fan", bedroom1Fan);
    printDualSchedule("Tube", bedroom1Tube);
    printDualSchedule("Bulb", bedroom1Bulb);
    printDualSchedule("Socket", bedroom1Socket);
    printDualSchedule("AC", bedroom1AC);
    Serial.println("=================================");
}

void initDeviceCache()
{
    loadConfiguration();
    printRestoredConfiguration();
}