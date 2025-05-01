#include "time_esp8266.h"
#ifdef ESP8266


unsigned long getCurrentTime(){
    return millis();
}


#endif
