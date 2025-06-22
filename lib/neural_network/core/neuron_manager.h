#ifndef NEURON_MANAGER_H
#define NEURON_MANAGER_H

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "neuron_core.h"
#include "../message_types.h"


typedef uint32_t BitField;


void handleNeuralNetworkMessage(char* messageBuffer);


#endif //NEURON_MANAGER_H
