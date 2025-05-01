#ifdef ESP32
#include "time_esp32.h"

unsigned long getCurrentTime(){
    return millis();
}

#endif