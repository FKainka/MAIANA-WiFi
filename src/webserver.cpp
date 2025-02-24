// general
#include <Arduino.h>
#include <ArduinoJson.h>

// webserver
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <AsyncTCP.h>
#include <AsyncUDP.h>
#include <ESPAsyncWebServer.h>

#include "helperfunctions.h"
#include "webserver.h"
#include "config.h"

extern void startConfigWiFi();
extern void startNMEAForward();
extern void stopNMEAForward();

// Definition der Webserver-Variablen
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/config.html", "text/html");
    });

    server.on("/dashboard.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/dashboard.html", "text/html");
    });
    server.serveStatic("/js", SPIFFS, "/js/");

    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) { handleInfo(request); });
    server.on("/txstate", HTTP_GET, [](AsyncWebServerRequest *request) { handleTXstate(request); });
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) { handleScan(request); });
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) { handleWifi(request); });
    server.on("/protocol", HTTP_GET, [](AsyncWebServerRequest *request) { handleProtocol(request); });
    server.on("/station", HTTP_GET, [](AsyncWebServerRequest *request) { handleStation(request); });
    server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) { handleSystem(request); });
    server.on("/app", HTTP_GET, [](AsyncWebServerRequest *request) { handleApp(request); });

    server.onNotFound(notFound);
}


void handleInfo(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json["ip"] = infoState.ip;
    json["configTimeout"] = infoState.configTimeout;
    json["time"] = infoState.time;
    json["date"] = infoState.date;
    json["signal"] = WiFi.RSSI();
    json["wifiReconnects"] = infoState.wifiReconnects;
    json["mode"] = infoState.mode;

    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
}

void handleTXstate(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_TOGGLE)) {
        if (txState.softwareSwitch) {
            Serial2.print("tx off\r\n");
        } else {
            Serial2.print("tx on\r\n");
        }
        txRefreshPending = true;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json["hardwarePresent"] = txState.hardwarePresent;
    json["hardwareSwitch"] = txState.hardwareSwitch;
    json["softwareSwitch"] = txState.softwareSwitch;
    json["stationData"] = txState.stationData;
    json["status"] = txState.status;
    json["channelALast"] = txState.channelALast;
    json["channelALastTime"] = txState.channelALastTime;
    json["channelALastDate"] = txState.channelALastDate;
    json["channelBLast"] = txState.channelBLast;
    json["channelBLastTime"] = txState.channelBLastTime;
    json["channelBLastDate"] = txState.channelBLastDate;
    json["channelANoise"] = txState.channelANoise;
    json["channelBNoise"] = txState.channelBNoise;

    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
}

void handleScan(AsyncWebServerRequest *request) {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
        WiFi.scanNetworks(true);
    } else if (n) {
        for (int i = 0; i < n; ++i) {
            if (i) {
                json += ",";
            }
            json += "{";
            json += "\"rssi\":" + String(WiFi.RSSI(i));
            json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
            json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
            json += ",\"channel\":" + String(WiFi.channel(i));
            json += ",\"secure\":" + String(WiFi.encryptionType(i));
            json += "}";
        }
        WiFi.scanDelete();
        if (WiFi.scanComplete() == -2) {
            WiFi.scanNetworks(true);
        }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
}

void handleWifi(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_SSID) && request->hasParam(PARAM_PASSWORD)) {
        wifiSettings.type = request->getParam(PARAM_TYPE)->value();
        wifiSettings.ssid = request->getParam(PARAM_SSID)->value();
        wifiSettings.password = request->getParam(PARAM_PASSWORD)->value();
        startConfigWiFi();
        startWebServer();
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json[PARAM_TYPE] = wifiSettings.type;
    json[PARAM_SSID] = wifiSettings.ssid;
    json[PARAM_PASSWORD] = wifiSettings.password;

    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
    if (request->params() == 3) {
        writeJsonFile(WIFI_SETTINGS_FILE, json);
    }
}

void handleApp(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_DEMOMODE)) {
        appSettings.demoMode = request->getParam(PARAM_DEMOMODE)->value().equals("true");
    }
    if (request->hasParam(PARAM_CORS)) {
        appSettings.corsHeader = request->getParam(PARAM_CORS)->value().equals("true");
    }
    if (request->hasParam(PARAM_AISMEM)) {
        appSettings.aisMemory = request->getParam(PARAM_AISMEM)->value().equals("true");
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json[PARAM_VERSION] = FIRMWARE_VERSION;
    json[PARAM_BUILD] = FIRMWARE_BUILD;
#ifdef FAKEAIS
    json[PARAM_DEMOMODE] = appSettings.demoMode;
#else
    json[PARAM_DEMOMODE] = false;
#endif
#ifdef CORS_HEADER
    json[PARAM_CORS] = appSettings.corsHeader;
#else
    json[PARAM_CORS] = false;
#endif
#ifdef AISMEMORY
    json[PARAM_AISMEM] = appSettings.aisMemory;
#else
    json[PARAM_AISMEM] = false;
#endif
    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
    if (request->params() == 3) {
        writeJsonFile(APP_SETTINGS_FILE, json);
    }
}

void handleProtocol(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_TCP_ENABLE) && request->hasParam(PARAM_TCP_PORT)) {
        protocolSettings.tcp = request->getParam(PARAM_TCP_ENABLE)->value().equals("true");
        auto port = request->getParam(PARAM_TCP_PORT)->value().toInt();
        if (portIsValid(port)) {
            protocolSettings.tcpPort = port;
        }
    }
    if (request->hasParam(PARAM_UDP_ENABLE) && request->hasParam(PARAM_UDP_PORT)) {
        protocolSettings.udp = request->getParam(PARAM_UDP_ENABLE)->value().equals("true");
        auto port = request->getParam(PARAM_UDP_PORT)->value().toInt();
        if (portIsValid(port)) {
            protocolSettings.udpPort = port;
        }
    }
    if (request->hasParam(PARAM_WEBSOCKET_ENABLE) && request->hasParam(PARAM_WEBSOCKET_PORT)) {
        protocolSettings.websocket = request->getParam(PARAM_WEBSOCKET_ENABLE)->value().equals("true");
        auto port = request->getParam(PARAM_WEBSOCKET_PORT)->value().toInt();
        if (portIsValid(port)) {
            protocolSettings.websocketPort = port;
        }
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json[PARAM_TCP_ENABLE] = protocolSettings.tcp;
    json[PARAM_TCP_PORT] = protocolSettings.tcpPort;
    json[PARAM_UDP_ENABLE] = protocolSettings.udp;
    json[PARAM_UDP_PORT] = protocolSettings.udpPort;
    json[PARAM_WEBSOCKET_ENABLE] = protocolSettings.websocket;
    json[PARAM_WEBSOCKET_PORT] = protocolSettings.websocketPort;

    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
    if (request->params() == 6) {
        writeJsonFile(PROTOCOL_SETTINGS_FILE, json);
        stopNMEAForward();
        startNMEAForward();
    }
}

void handleStation(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_MMSI) && request->hasParam(PARAM_CALLSIGN) && request->hasParam(PARAM_VESSELNAME) &&
        request->hasParam(PARAM_VESSELTYPE) && request->hasParam(PARAM_LOA) && request->hasParam(PARAM_BEAM) &&
        request->hasParam(PARAM_PORTOFFSET) && request->hasParam(PARAM_BOWOFFSET)) {
        stationSettings.mmsi = request->getParam(PARAM_MMSI)->value();
        stationSettings.callsign = request->getParam(PARAM_CALLSIGN)->value();
        stationSettings.vesselname = request->getParam(PARAM_VESSELNAME)->value();
        stationSettings.vesseltype = request->getParam(PARAM_VESSELTYPE)->value().toInt();
        stationSettings.loa = request->getParam(PARAM_LOA)->value().toInt();
        stationSettings.beam = request->getParam(PARAM_BEAM)->value().toInt();
        stationSettings.bowoffset = request->getParam(PARAM_BOWOFFSET)->value().toInt();
        stationSettings.portoffset = request->getParam(PARAM_PORTOFFSET)->value().toInt();

        Serial2.println("station " + stationSettings.mmsi + "," + stationSettings.mmsi + "," + stationSettings.callsign +
                        "," + stationSettings.vesselname + "," + stationSettings.vesseltype + "," + stationSettings.loa +
                        "," + stationSettings.beam + "," + stationSettings.bowoffset + "," + stationSettings.portoffset);
    }
    stationRefreshPending = true;
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json["mmsi"] = stationSettings.mmsi;
    json["callsign"] = stationSettings.callsign;
    json["vesselname"] = stationSettings.vesselname;
    json["vesseltype"] = stationSettings.vesseltype;
    json["loa"] = stationSettings.loa;
    json["beam"] = stationSettings.beam;
    json["portoffset"] = stationSettings.portoffset;
    json["bowoffset"] = stationSettings.bowoffset;

    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
}

void handleSystem(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json["hardwareRevision"] = systemSettings.hardwareRevision;
    json["firmwareRevision"] = systemSettings.firmwareRevision;
    json["serialNumber"] = systemSettings.serialNumber;
    json["MCUtype"] = systemSettings.MCUtype;
    json["breakoutGeneration"] = systemSettings.breakoutGeneration;
    json["bootloader"] = systemSettings.bootloader;

    serializeJson(json, *response);
    addOptionalCORSHeader(response);
    request->send(response);
    systemRefreshPending = true;
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void addOptionalCORSHeader(AsyncResponseStream *response) {
#ifdef CORS_HEADER
    if (appSettings.corsHeader) {
        response->addHeader("Access-Control-Allow-Origin", "*");
    }
#endif
}

void websocketSend(const char *line) {
    if (websocketOk) {
        ws.textAll(line);
        if (debug_logging) {
            Serial.print("Websocket send: ");
            Serial.println(line);
        }
    }
}

void sendHistoryWSChuncked(uint32_t client, long msgId) {
#ifdef AISMEMORY
    std::map<std::string, std::string> ais_messages = getAISMessages();
    std::map<std::string, std::string>::iterator it;
    Serial.printf("Send %u history to WebSocket client #%u. from # %u\n", ais_messages.size(), client, msgId);
    long i = 0;

    for (it = ais_messages.begin(); it != ais_messages.end(); it++) {
        i++;
        if (i < msgId) {
            continue;
        }
        Serial.print(it->first.c_str());
        Serial.print(" : ");
        Serial.println(it->second.c_str());

        if (ws.availableForWrite(client)) {
            ws.text(client, it->second.c_str());
        }
        ws_queue_loop = millis();
        if (i > msgId + WS_MAX_MESSAGES) {
            ws_queue_msgId = i;
            return;
        }
    }
    ws_queue_msgId = -1;
    ws_queue_client = 0;
#endif
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
#ifdef AISMEMORY
            ws_queue_client = client->id();
            ws_queue_msgId = 0;
            ws_queue_loop = millis();
#endif
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            break;
        case WS_EVT_PONG:
            Serial.printf("WebSocket client #%u pong\n", client->id());
            break;
        case WS_EVT_ERROR:
            Serial.printf("WebSocket client #%u event error:\n", client->id());
            break;
    }
}
