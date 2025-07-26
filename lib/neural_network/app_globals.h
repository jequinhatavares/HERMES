#ifndef WIFIROUTING_APP_GLOBALS_H
#define WIFIROUTING_APP_GLOBALS_H

#include <network.h>

extern class Network network;

extern char appPayload[200];
extern char appBuffer[250];

typedef enum class DeviceType{
    DEVICE_ESP8266,
    DEVICE_ESP32,
    DEVICE_RPI,
}DeviceType;

extern DeviceType deviceType;  // Just declare it here, don't define



#endif //WIFIROUTING_APP_GLOBALS_H
