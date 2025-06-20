#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <cstring>
#include <logger.h>
#include <table.h>
#include "../nn_configurations.h"

typedef struct NeuronInfo{
    int* saveOrder = nullptr;
    float* weights = nullptr;
    float* inputs = nullptr;
    int bias = 0;
    int inputSize = 0;
}NeuronInfo;

bool isNeuronEqual(void* av, void* bv);
void setNeuronID(void* av, void* bv);
void setNeuronInfo(void* av, void* bv);

void configureNeuron(int neuronId, int receivedInputSize, float* receivedWeights, float receivedBias, int* receivedOrder);
void setInput(int neuronId,float inputValue, int inputID);
float computeNeuronOutput(int neuronId);

#endif //NEURAL_NETWORK_H
