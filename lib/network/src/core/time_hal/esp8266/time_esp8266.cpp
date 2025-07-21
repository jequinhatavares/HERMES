#include "time_esp8266.h"
#ifdef ESP8266

/**
 * getCurrentTime
 * Calculates the elapsed time in milliseconds since the program start.
 *
 * @return (unsigned long) - The elapsed time in milliseconds
 */
unsigned long getCurrentTime(){
    return millis();
}


#endif
