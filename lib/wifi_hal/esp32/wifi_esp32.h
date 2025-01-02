#ifndef ESP32WIFI_H
#define ESP32WIFI_H
#ifdef ESP32

#include <WiFi.h>

#include "../wifi_hal.h"

void onSoftAPModeStationConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onSoftAPModeStationDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onStationModeGotIPHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onStationModeConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onStationModeDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void initWifiEventHandlers();

String Get_WiFiStatus(int Status);

void startWifiSTA(const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet, const IPAddress& dns);

void startWifiAP(const char* SSID, const char* PASS, const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet);

List searchAP(String);

void connectToAP(const char*, const char*);

void stopWifiAP();

int numberOfSTAConnected();

IPAddress getGatewayIP();

IPAddress getMySTAIP();

String getMyMAC();

IPAddress getMyAPIP();

void changeWifiMode(int);

#endif//ESP32
#endif //ESP32WIFI_H
