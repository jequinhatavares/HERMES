
#ifndef WIFI_H
#define WIFI_H

#include "logger.h"
//#include "lifecycle.h"

#define disconnectionThreshold 3

extern bool initializeAP;

typedef struct List {
    char item[10][50];
    int len = 0;
} List;

extern List ssidList;

extern unsigned long lastDisconnectionTime;
extern int parentDisconnectionCount;

extern void (*parentDisconnectCallback)();

extern int childDisconnectionCount;

#if defined(ESP8266)
    #include "esp8266/wifi_esp8266.h"
#endif

#if defined(ESP32)
    #include "esp32/wifi_esp32.h"
#endif


#endif //WIFI_H