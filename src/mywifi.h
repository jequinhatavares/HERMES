
#ifndef WIFI_H
#define WIFI_H

#define SSID_PREFIX      		"JessicaNode"
#define PASS      		        "123456789"
#define SERVER_IP_ADDR			"192.168.4.1"
#define SERVER_PORT				4011


#include <WiFiClient.h>
#include <WiFiServer.h>

IPAddress myIP;

#if defined(ESP32)
#include <WiFi.h>
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif  // ESP32

String Get_WiFiStatus(int Status);

void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info);

void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info);

void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info);

void startWifiAP();

void searchAP();

bool sendMessage(String message, WiFiClient curr_client);


#endif //WIFI_H