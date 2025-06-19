#ifndef NEURON_MANAGER_H
#define NEURON_MANAGER_H

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "neural_network.h"


typedef enum NeuralNetworkMessageType{
    NN_ASSIGN_NEURON, //O
    NN_NEURON_OUTPUT, //1
}NeuralNetworkMessageType;


void handleNeuralNetworkMessage(char* messageBuffer);


#endif //NEURON_MANAGER_H
