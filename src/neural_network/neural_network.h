#ifndef WIFIROUTING_NEURAL_NETWORK_H
#define WIFIROUTING_NEURAL_NETWORK_H

#include "coordinator/neural_network_manager.h"
#include "worker/neuron_manager.h"
#include "../network_instance.h"



void handleNeuralNetworkMessage(uint8_t* originatorIP,uint8_t* destinationIP,char* messageBuffer);
void neuralNetworkOnTimer();



#endif //WIFIROUTING_NEURAL_NETWORK_H
