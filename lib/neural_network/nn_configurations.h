#ifndef NN_CONFIGURATIONS_H
#define NN_CONFIGURATIONS_H


#if defined(ESP8266)
#define MAX_NEURONS 2
#endif


#if defined(ESP32)
#define MAX_NEURONS 5
#endif


#if defined(raspberrypi_3b)
#define MAX_NEURONS 3
#endif


#endif //NN_CONFIGURATIONS_H
