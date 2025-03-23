#include "logger.h"


bool activeLogModules[MAX_MODULES]  = {false};

LogLevels currentLogLevel = ERROR;

void enableModule(LogModules module) {
    activeLogModules[module] = true;
}

void disableModule(LogModules module) {
    activeLogModules[module] = false;
}

bool isModuleEnabled(LogModules module) {
    return activeLogModules[module];
}

void logHeaders(LogModules module){

}
void logMessage(LogModules module, LogLevels level, const char* format, ...) {
    printf("Start logger\n");

    //The module is printed only if it is included in the modules to print
    if (!isModuleEnabled(module)) {
        return;
    }

    //The message is printed only if its level is below the current logging level
    if (level < currentLogLevel) {
        return;
    }

    va_list args;
    va_start(args,format); //Initialize argument list

    //LOG_FUNCTION(format, args);

    #if defined(ESP32) || defined(ESP8266)
        #include <Arduino.h>
        char buffer[500];
        vsnprintf(buffer, sizeof(buffer),format,args);
        Serial.printf("%s", buffer);
    #endif

    //time_t now = time(NULL); //for time prints
    //Serial.printf("[%s][%s] %s\n", ctime(&now), module, text);

    va_end(args);
}
