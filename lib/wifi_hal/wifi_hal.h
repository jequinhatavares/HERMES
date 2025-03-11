
#ifndef WIFI_H
#define WIFI_H


extern bool initializeAP;

//IPAddress myIP;

typedef struct List {
    char* item[10];
    int len = 0;
} List;

extern List ssidList;

#if defined(ESP8266)
    #include "esp8266/wifi_esp8266.h"
#endif

#if defined(ESP32)
    #include "esp32/wifi_esp32.h"
#endif


#endif //WIFI_H