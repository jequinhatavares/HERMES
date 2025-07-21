#ifdef ESP32
#include "time_esp32.h"

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