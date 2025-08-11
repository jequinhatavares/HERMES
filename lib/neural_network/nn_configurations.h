#ifndef NN_CONFIGURATIONS_H
#define NN_CONFIGURATIONS_H


#define TOTAL_NEURONS 12

#define TOTAL_INPUT_NEURONS 2


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

/*** This variable defines the maximum number of input neurons a node can host, which corresponds to the maximum
 *   number of sensors it can integrate. Each sensor maps to exactly one input neuron.***/
#define MAX_INPUT_NEURONS 1

#define MAX_TARGET_OUTPUTS 4

#define NACK_TIMEOUT 3000

#define INPUT_WAIT_TIMEOUT 3000

#define ACK_TIMEOUT 3000

#define INFERENCE_TIMEOUT 3000

#define MIN_WORKERS 3



#endif //NN_CONFIGURATIONS_H
