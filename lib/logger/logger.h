#ifndef LOGGER_H
#define LOGGER_H

#if defined(ESP32) || defined(ESP8266)
#include <Arduino.h>
#endif

#ifdef native
#include <cstdio>
#endif

#include <stdarg.h>
#include <cstring>

#define MAX_MODULES 10

typedef enum LogModules {
    NETWORK, //0
    MESSAGES, //1
    STATE_MACHINE, //2
    DEBUG_SERVER, //3
}LogModules;

typedef enum LogLevels{
    DEBUG, //0
    INFO, //1
    ERROR, //2
}LogLevels;

extern bool activeLogModules[MAX_MODULES];

extern LogLevels currentLogLevel;

extern LogModules lastModule;


void enableModule(LogModules module);
void disableModule(LogModules module);
bool isModuleEnabled(LogModules module);
void logHeaders(LogModules module);
void LOG(LogModules module, LogLevels level, const char* format, ...);

#endif //LOGGER_H
