#ifndef ESP8266WIFI_H
#define ESP8266WIFI_H
#ifdef ESP8266
#include <ESP8266WiFi.h>

#include "../wifi_hal.h"


void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info);

void onSoftAPModeStationDisconnectedHandler(const WiFiEventSoftAPModeStationDisconnected& info);

void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info);

void onStationModeConnectedHandler(const WiFiEventStationModeConnected& info);

void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info);

void initWifiEventHandlers();

String Get_WiFiStatus(int Status);

void startWifiSTA(const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet, const IPAddress& dns);

void startWifiAP(const char* SSID, const char* Pass, const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet);

void searchAP(String);

void connectToAP(const char*, const char*);

void stopWifiAP();

void disconnectFromAP();

int numberOfSTAConnected();

IPAddress getGatewayIP();

IPAddress getMySTAIP();

String getMyMAC();

IPAddress getMyAPIP();

void changeWifiMode(int);
#endif
#endif //ROUTING_ESP8266WIFI_H
