
#ifndef WIFI_H
#define WIFI_H
#include <WiFiClient.h>
#include <WiFiServer.h>

extern WiFiClient parent;
extern bool initializeAP;

//IPAddress myIP;

typedef struct List_ {
    String item[10];
    int len = 0;
} List;


#if defined(ESP8266)
    #include "esp8266/wifi_esp8266.h"
#endif

#if defined(ESP32)
    #include "esp32/wifi_esp32.h"
#endif


#endif //WIFI_H