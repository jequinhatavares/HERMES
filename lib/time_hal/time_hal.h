#ifndef TIME_HAL_H
#define TIME_HAL_H


#if defined(ESP8266)
    #include "esp8266/time_esp8266.h"
#endif

#if defined(ESP32)
    #include "esp32/time_esp32.h"
#endif

#if defined(raspberrypi_3b)
#include "raspberrypi/time_raspberrypi.h"
#endif

//#if defined(raspberrypi_3b)
//#include "raspberrypi/wifi_raspberrypi.h"
//#endif


#endif //TIME_HAL_H
