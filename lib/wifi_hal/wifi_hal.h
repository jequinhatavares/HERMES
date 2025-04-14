#ifndef WIFI_H
#define WIFI_H

//#include "lifecycle.h"


#if defined(ESP8266)
    #include "esp8266/wifi_esp8266.h"
   // #include <ESP8266WiFi.h>
#endif

#if defined(ESP32)
    #include "esp32/wifi_esp32.h"
#endif


#endif //WIFI_H