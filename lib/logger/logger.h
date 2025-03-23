#ifndef LOGGER_H
#define LOGGER_H

#if defined(ESP32) || defined(ESP8266)
#include <Arduino.h>
#define LOG_FUNCTION Serial.printf
#endif

#ifdef native
#define LOG_FUNCTION vprintf
#endif

#include <stdarg.h>
#include <cstdio>
#include <cstring>

#define MAX_MODULES 10

typedef enum LogModules {
    STATE_MACHINE, //0
    MESSAGES, //1
    NETWORK, //2
}LogModules;

typedef enum LogLevels{
    ERROR, //0
    INFO, //1
    DEBUG, //2
}LogLevels;

extern bool activeLogModules[MAX_MODULES];

extern LogLevels currentLogLevel;


void enableModule(LogModules module);
void disableModule(LogModules module);
bool isModuleEnabled(LogModules module);
void logMessage(LogModules module, LogLevels level, const char* format, ...);

#endif //LOGGER_H
