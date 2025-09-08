#ifndef ESP32WIFI_H
#define ESP32WIFI_H
#ifdef ESP32

#include <WiFi.h>

#ifndef WIFI_CONNECTION_TIMEOUT
#define WIFI_CONNECTION_TIMEOUT_ESP32 3000
#endif

#include "../wifi_interface.h"

void onSoftAPModeStationConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onSoftAPModeStationDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onStationModeGotIPHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onStationModeConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);

void onStationModeDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);


#endif//ESP32
#endif //ESP32WIFI_H
