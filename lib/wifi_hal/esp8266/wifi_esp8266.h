#ifndef ESP8266WIFI_H
#define ESP8266WIFI_H
#ifdef ESP8266
#include <ESP8266WiFi.h>

void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info);

void onSoftAPModeStationDisconnectedHandler(const WiFiEventSoftAPModeStationDisconnected& info);

void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info);

void onStationModeConnectedHandler(const WiFiEventStationModeConnected& info);

void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info);

void initWifiEventHandlers();
#endif
#endif //ROUTING_ESP8266WIFI_H
