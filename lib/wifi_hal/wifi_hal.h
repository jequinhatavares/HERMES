
#ifndef WIFI_H
#define WIFI_H

#define SSID_PREFIX      		"JessicaNode"
#define PASS      		        "123456789"
#define SERVER_IP_ADDR			"192.168.4.1"
#define SERVER_PORT				4011


#include <WiFiClient.h>
#include <WiFiServer.h>

#if defined(ESP8266)
    #include "esp8266/wifi_esp8266.h"
#endif

#if defined(ESP32)
    #include "esp32/wifi_esp32.h"
#endif


String Get_WiFiStatus(int Status);

void startWifiAP();

void searchAP();

bool sendMessage(String message, WiFiClient curr_client);


#endif //WIFI_H