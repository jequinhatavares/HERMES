#include "logger.h"


bool activeLogModules[MAX_MODULES]  = {false};

LogLevels currentLogLevel = ERROR;

LogModules lastModule;

/**
 * enableModule
 * Enables logging for a specific module.
 *
 * @param module- The module to enable logging for.
 *
 * @return void
 */
void enableModule(LogModules module) {
    activeLogModules[module] = true;
}


/**
 * enableAllModules
 * Enables all logging modules
 *
 * @param module- The module to enable logging for.
 *
 * @return void
 */
void enableAllModules() {
    for (int i = 0; i < MAX_MODULES; i++) {
        activeLogModules[i] = true;
    }
}

/**
 * disableModule
 * Disables logging for a specific module.
 *
 * @param module - The module whose logging should be disabled
 *
 * @return void
 */
void disableModule(LogModules module) {
    activeLogModules[module] = false;
}

/**
 * isModuleEnabled
 * Checks if logging is enabled for a specific module.
 *
 * @param module - The module to check.
 *
 * @return bool - True if logging is enabled for the module, false otherwise.
 */
bool isModuleEnabled(LogModules module) {
    return activeLogModules[module];
}

/**
 * logHeaders
 * Prints a unique header for the given module to visually separate log sections.
 * Ensures that a header is printed only when the module changes to avoid redundancy.
 *
 * @param module - The module for which to print a header.
 *
 * @return void
 */
void logHeaders(LogModules module){

    if(lastModule == module)return;
    lastModule = module;

    switch (module) {
        case NETWORK:
        #if defined(ESP32) || defined(ESP8266)
            Serial.printf("\n--------------------- NETWORK ----------------------\n");
        #endif
        #ifdef NATIVE
            printf("\n---------------------NETWORK----------------------\n");
        #endif
            break;
        case MESSAGES:
        #if defined(ESP32) || defined(ESP8266)
                    Serial.printf("\n««««««««««««««««««««« MESSAGES »»»»»»»»»»»»»»»»»»»»\n");
        #endif
        #ifdef NATIVE
                    printf("\n««««««««««««««««««««« MESSAGES »»»»»»»»»»»»»»»»»»»»\n");
        #endif
            break;
        case STATE_MACHINE:
        #if defined(ESP32) || defined(ESP8266)
                    Serial.printf("\n:::::::::::::::::::::: STATE MACHINE ::::::::::::::::::::::\n");
        #endif
        #ifdef NATIVE
                    printf("\n:::::::::::::::::::::: STATE MACHINE ::::::::::::::::::::::\n");
        #endif
            break;
    }
}
void logSimplePrint(const char* msg){
    #if defined(ESP32) || defined(ESP8266)
        Serial.printf("%s",msg);
    #endif
    #if defined(NATIVE) || defined(raspberrypi_3b)
        printf("%s",msg);
    #endif
}
/**
 * LOG
 * Logs a formatted message if the specified module is enabled and the log level
 * is within the current logging threshold. Supports variable arguments
 * for dynamic message formatting.
 *
 * @param module - The module associated with the log message.
 * @param level - The logging level of the message (e.g., DEBUG, INFO, ERROR).
 * @param format - The format string, similar to printf.
 * @param ... - Arguments corresponding to the format specifiers.
 *
 * @return void
 *
 * @example
 * // Example usage:
 * LOG(NETWORK, INFO, "Connected to node with IP: %d.%d.%d.%d\n", 192, 168, 1, 1);
 * LOG(MESSAGES, ERROR, "Failed to send message: %s\n", errorMessage);
 */
void LOG(LogModules module, LogLevels level, const char* format, ...) {
    //The module is printed only if it is included in the modules to print
    if (!isModuleEnabled(module)) {
        return;
    }

    //The message is printed only if its level is below the current logging level
    if (level < currentLogLevel) {
        return;
    }

    /***
    if(module == STATE_MACHINE) logSimplePrint("[SM] ");
    else if(module == MESSAGES)logSimplePrint("[M] ");
    else if(module == NETWORK)logSimplePrint("[N] ");
    else if(module == DEBUG_SERVER)logSimplePrint("[D] ");
    ***/
    va_list args;
    va_start(args,format); //Initialize argument list

    //logHeaders(module);
    #if defined(ESP32) || defined(ESP8266)
        char buffer[500];
        vsnprintf(buffer, sizeof(buffer),format,args);
        Serial.printf("%s", buffer);
    #endif

    #if defined(NATIVE) || defined(raspberrypi_3b)
        vprintf(format, args);
    #endif

    //time_t now = time(NULL); //for time prints
    //Serial.printf("[%s][%s] %s\n", ctime(&now), module, text);
    va_end(args);
}
