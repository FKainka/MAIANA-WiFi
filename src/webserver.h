#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <AsyncUDP.h>

// Deklarationen der Webserver-Funktionen und -Variablen
extern AsyncWebServer server;
extern AsyncWebSocket ws;

bool mDNSOK = false;
bool networkOK = false;
bool udpForwardOK = false;
bool tcpForwardOK = false;
bool websocketOk = false;

#ifdef AISMEMORY  // Stores the latest10 minutes of unique AIS messages and
                  // resend on connect of a client
#include "aisstore.h"
uint32_t ws_queue_client = 0;
long ws_queue_msgId = -1;
long ws_queue_loop = millis() + 1000L;
#define WS_MAX_MESSAGES 20

#endif


void setupWebServer();
void startWebServer();
void stopWebServer();
void initWebSocket();
void handleInfo(AsyncWebServerRequest *request);
void handleTXstate(AsyncWebServerRequest *request);
void handleScan(AsyncWebServerRequest *request);
void handleWifi(AsyncWebServerRequest *request);
void handleProtocol(AsyncWebServerRequest *request);
void handleStation(AsyncWebServerRequest *request);
void handleSystem(AsyncWebServerRequest *request);
void handleApp(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);
void addOptionalCORSHeader(AsyncResponseStream *response);
void websocketSend(const char *line);
void sendHistoryWSChuncked(uint32_t client, long msgId);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

#endif // WEBSERVER_H
