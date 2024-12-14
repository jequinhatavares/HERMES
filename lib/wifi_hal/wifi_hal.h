
#ifndef WIFI_H
#define WIFI_H



#include <WiFiClient.h>
#include <WiFiServer.h>

extern WiFiClient parent;
extern bool initializeAP;

#if defined(ESP8266)
    #include "esp8266/wifi_esp8266.h"
#endif

#if defined(ESP32)
    #include "esp32/wifi_esp32.h"
#endif

typedef struct List{
    String item[10];
    int len = 0;
} List;

String Get_WiFiStatus(int Status);

void startWifiAP(const char* SSID, const char* PASS);

List searchAP(String);

void connectToAP(const char*, const char*);

void stopWifiAP();

int numberOfSTAConnected();

IPAddress getGatewayIP();

IPAddress getMySTAIP();

IPAddress getMyAPIP();

String getMyMAC();

void changeWifiMode(int);

#endif //WIFI_H