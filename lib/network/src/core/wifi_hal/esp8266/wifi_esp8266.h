#ifndef ESP8266WIFI_H
#define ESP8266WIFI_H
#ifdef ESP8266

#include <ESP8266WiFi.h>

#include "../wifi_interface.h"

#ifndef WIFI_CONNECTION_TIMEOUT
#define WIFI_CONNECTION_TIMEOUT_ESP8266 4000
#endif

void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info);
void onSoftAPModeStationDisconnectedHandler(const WiFiEventSoftAPModeStationDisconnected& info);
void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info);
void onStationModeConnectedHandler(const WiFiEventStationModeConnected& info);
void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info);


#endif
#endif //ROUTING_ESP8266WIFI_H
