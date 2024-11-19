
#ifndef WIFI_H
#define WIFI_H

//#include <ESP8266WiFi.h>

#if defined(ESP32)
#include <WiFi.h>
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif  // ESP32


#endif //WIFI_H