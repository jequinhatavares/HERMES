#ifndef WIFIROUTING_NEURAL_NETWORK_DISPATCHER_H
#define WIFIROUTING_NEURAL_NETWORK_DISPATCHER_H

#include "coordinator/neural_network_coordinator.h"
#include "worker/neuron_manager.h"
#include "app_globals.h"


void onNetworkJoin(uint8_t *parentIP);

void handleNeuralNetworkMessage(uint8_t* originatorIP,uint8_t* destinationIP,char* messageBuffer);
void neuralNetworkOnTimer();



#endif //WIFIROUTING_NEURAL_NETWORK_DISPATCHER_H
