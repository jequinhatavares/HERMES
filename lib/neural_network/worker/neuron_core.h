#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <cstring>
//#include <logger.h>
#include "../nn_configurations.h"
#include "../nn_types.h"

#include "../app_globals.h"



extern NeuronId neuronIds[MAX_NEURONS];
extern float* weights[MAX_NEURONS];
extern float* inputs[MAX_NEURONS];
extern NeuronId* saveOrders[MAX_NEURONS];
extern float biases[MAX_NEURONS];
extern uint8_t inputSizes[MAX_NEURONS];

extern int neuronsCount;


void configureNeuron(NeuronId neuronId, uint8_t receivedInputSize, float* receivedWeights, float receivedBias, NeuronId* receivedOrder);

int getNeuronStorageIndex(NeuronId neuronId);
int getInputStorageIndex(NeuronId neuronId, NeuronId inputId);
int getInputSize(NeuronId neuronId);

bool isInputRequired(NeuronId neuronId,NeuronId inputId);
bool computesNeuron(NeuronId neuronId);

void setInput(NeuronId neuronId,float inputValue, NeuronId sourceNodeId);
float computeNeuronOutput(NeuronId neuronId);
void freeAllNeuronMemory();

bool isNeuronInList(NeuronId *neuronsList, uint8_t nNeurons, NeuronId targetNeuronId);
void printNeuronInfo();

#endif //NEURAL_NETWORK_H
