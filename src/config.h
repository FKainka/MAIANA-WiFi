#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#define LED_BUILTIN 2

#define SWITCH 4

// in seconds
#define CONFIG_TIMEOUT 300

#define BLINK_SEC 2

// Set config WiFi credentials
#define CONFIG_SSID "Wokwi-GUEST"
#define CONFIG_PASS ""
#define WIFI_SETTINGS_FILE "/wifi.json"
#define PROTOCOL_SETTINGS_FILE "/protocol.json"
#define APP_SETTINGS_FILE "/app.json"

#define NMEALEN 84

#define WIFICONFIGLINELEN 100

const char *ssidWebserver = "MAIANA";
const char *passwordWebserver = "MAIANA-AIS";

const char *PARAM_TYPE = "type";
const char *PARAM_SSID = "ssid";
const char *PARAM_PASSWORD = "password";

const char *PARAM_TCP_ENABLE = "tcp";
const char *PARAM_TCP_PORT = "tcpport";
const char *PARAM_UDP_ENABLE = "udp";
const char *PARAM_UDP_PORT = "udpport";
const char *PARAM_WEBSOCKET_ENABLE = "websocket";
const char *PARAM_WEBSOCKET_PORT = "websocketport";

const char *PARAM_MMSI = "mmsi";
const char *PARAM_CALLSIGN = "callsign";
const char *PARAM_VESSELNAME = "vesselname";
const char *PARAM_VESSELTYPE = "vesseltype";
const char *PARAM_LOA = "loa";
const char *PARAM_BEAM = "beam";
const char *PARAM_PORTOFFSET = "portoffset";
const char *PARAM_BOWOFFSET = "bowoffset";
const char *PARAM_TOGGLE = "softtxtoggle";

const char *PARAM_VERSION = "version";
const char *PARAM_BUILD = "build";
const char *PARAM_DEMOMODE = "demoMode";
const char *PARAM_CORS = "corsHeader";
const char *PARAM_AISMEM = "aisMemory";

bool txRefreshPending = true;
bool systemRefreshPending = true;
bool stationRefreshPending = true;

bool debug_logging = false;

struct wifi {
    String type;
    String ssid;
    String password;
} wifiSettings;

struct protocol {
    bool tcp;
    int tcpPort;
    bool udp;
    int udpPort;
    bool websocket;
    int websocketPort;
} protocolSettings;

struct station {
    String mmsi;
    String callsign;
    String vesselname;
    int vesseltype;
    int loa;
    int beam;
    int portoffset;
    int bowoffset;
} stationSettings;

struct system {
    String hardwareRevision;
    String firmwareRevision;
    String serialNumber;
    String MCUtype;
    String breakoutGeneration;
    String bootloader;
} systemSettings;


struct appSettings {
    bool demoMode;
    bool corsHeader;
    bool aisMemory;
} appSettings;


struct info {
    String ip;
    String configTimeout;
    String time;
    String date;
    String mode;
    long wifiReconnects;
} infoState;

struct txState {
    String hardwarePresent;
    String hardwareSwitch;
    String softwareSwitch;
    String stationData;
    String status;
    String channelALast;
    String channelALastTime;
    String channelALastDate;
    String channelBLast;
    String channelBLastTime;
    String channelBLastDate;
    String channelANoise;
    String channelBNoise;
} txState;

#endif // CONFIG_H
