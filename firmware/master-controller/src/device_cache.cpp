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
    if (device.startHour > 23)
    {
        device.startHour = defaultConfig.startHour;
        valid = false;
    }
    if (device.startMinute > 59)
    {
        device.startMinute = defaultConfig.startMinute;
        valid = false;
    }
    if (device.stopHour > 23)
    {
        device.stopHour = defaultConfig.stopHour;
        valid = false;
    }
    if (device.stopMinute > 59)
    {
        device.stopMinute = defaultConfig.stopMinute;
        valid = false;
    }

    return valid;
}

void saveDeviceConfiguration(Preferences &prefs, const char* prefix, const DeviceConfig &device)
{
    char key[16];

    snprintf(key, sizeof(key), "%s_mode", prefix);
    putUCharIfChanged(prefs, key, device.mode);

    snprintf(key, sizeof(key), "%s_st_h", prefix);
    putUCharIfChanged(prefs, key, device.startHour);

    snprintf(key, sizeof(key), "%s_st_m", prefix);
    putUCharIfChanged(prefs, key, device.startMinute);

    snprintf(key, sizeof(key), "%s_sp_h", prefix);
    putUCharIfChanged(prefs, key, device.stopHour);

    snprintf(key, sizeof(key), "%s_sp_m", prefix);
    putUCharIfChanged(prefs, key, device.stopMinute);
}

bool loadDeviceConfiguration(Preferences &prefs, const char* prefix, DeviceConfig &device, const DeviceConfig &defaultConfig)
{
    char key[16];
    bool keysMissing = false;

    snprintf(key, sizeof(key), "%s_mode", prefix);
    if (prefs.isKey(key)) device.mode = prefs.getUChar(key);
    else { device.mode = defaultConfig.mode; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_st_h", prefix);
    if (prefs.isKey(key)) device.startHour = prefs.getUChar(key);
    else { device.startHour = defaultConfig.startHour; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_st_m", prefix);
    if (prefs.isKey(key)) device.startMinute = prefs.getUChar(key);
    else { device.startMinute = defaultConfig.startMinute; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_sp_h", prefix);
    if (prefs.isKey(key)) device.stopHour = prefs.getUChar(key);
    else { device.stopHour = defaultConfig.stopHour; keysMissing = true; }

    snprintf(key, sizeof(key), "%s_sp_m", prefix);
    if (prefs.isKey(key)) device.stopMinute = prefs.getUChar(key);
    else { device.stopMinute = defaultConfig.stopMinute; keysMissing = true; }

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
    DeviceConfig defaultFan    = { 2, false, 23, 0, 5, 0 };  // MODE_AUTO = 2
    DeviceConfig defaultTube   = { 2, false, 18, 0, 23, 0 };
    DeviceConfig defaultBulb   = { 2, false, 18, 0, 23, 0 };
    DeviceConfig defaultSocket = { 0, false, 0, 0, 0, 0 };   // MODE_OFF = 0
    DeviceConfig defaultAC     = { 0, false, 0, 0, 0, 0 };

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
    Serial.println("Schedules:");
    Serial.printf("Fan      %02u:%02u - %02u:%02u\n", bedroom1Fan.startHour, bedroom1Fan.startMinute, bedroom1Fan.stopHour, bedroom1Fan.stopMinute);
    Serial.printf("Tube     %02u:%02u - %02u:%02u\n", bedroom1Tube.startHour, bedroom1Tube.startMinute, bedroom1Tube.stopHour, bedroom1Tube.stopMinute);
    Serial.printf("Bulb     %02u:%02u - %02u:%02u\n", bedroom1Bulb.startHour, bedroom1Bulb.startMinute, bedroom1Bulb.stopHour, bedroom1Bulb.stopMinute);
    Serial.printf("Socket   %02u:%02u - %02u:%02u\n", bedroom1Socket.startHour, bedroom1Socket.startMinute, bedroom1Socket.stopHour, bedroom1Socket.stopMinute);
    Serial.printf("AC       %02u:%02u - %02u:%02u\n", bedroom1AC.startHour, bedroom1AC.startMinute, bedroom1AC.stopHour, bedroom1AC.stopMinute);
    Serial.println("=================================");
}

void initDeviceCache()
{
    loadConfiguration();
    printRestoredConfiguration();
}