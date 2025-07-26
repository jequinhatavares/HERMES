#ifndef NN_CONFIGURATIONS_H
#define NN_CONFIGURATIONS_H


#if defined(ESP8266)
#define MAX_NEURONS 2
#endif


#if defined(ESP32)
#define MAX_NEURONS 5
#endif


#if defined(raspberrypi_3b)
#define MAX_NEURONS 100
#endif

#if defined(NATIVE)
#define MAX_NEURONS 2
#endif

#define MAX_TARGET_OUTPUTS 4

#define NACK_TIMEOUT 3000

#define INPUT_WAIT_TIMEOUT 3000

#define ACK_TIMEOUT 3000

#define MIN_WORKERS 5



#endif //NN_CONFIGURATIONS_H
