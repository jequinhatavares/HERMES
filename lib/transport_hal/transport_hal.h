
#ifndef TRANSPORT_HAL_H
#define TRANSPORT_HAL_H

//#define UDP_PORT 5000

#if defined(ESP32)
    #include "esp32/udp_esp32.h"
#endif
#if defined(ESP8266)
    #include "esp8266/udp_esp8266.h"
#endif
#if defined(NATIVE)
    #include "PC/udp_pc.h"
#endif

#endif //TRANSPORT_HAL_H
