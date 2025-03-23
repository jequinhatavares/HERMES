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
void LOG(LogModules module, LogLevels level, const char* format, ...) {

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


    #if defined(ESP32) || defined(ESP8266)
        char buffer[500];
        vsnprintf(buffer, sizeof(buffer),format,args);
        Serial.printf("%s", buffer);
    #endif

    #ifdef native
        vprintf(format, args);
    #endif

    //time_t now = time(NULL); //for time prints
    //Serial.printf("[%s][%s] %s\n", ctime(&now), module, text);

    va_end(args);
}
