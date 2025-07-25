#ifndef WIFIROUTING_APP_GLOBALS_H
#define WIFIROUTING_APP_GLOBALS_H

#include <network.h>

extern class Network network;  // Just a declaration

extern char appPayload[200];
extern char appBuffer[250];

typedef enum class DeviceType{
    DEVICE_ESP8266,
    DEVICE_ESP32,
    DEVICE_RPI,
}DeviceType;

#ifdef ESP8266
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

#ifdef ESP32
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

#ifdef raspberrypi_3b
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

#endif //WIFIROUTING_APP_GLOBALS_H
