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

extern int neuronIds[MAX_NEURONS];
extern float* weights[MAX_NEURONS];
extern float* inputs[MAX_NEURONS];
extern int* saveOrders[MAX_NEURONS];
extern float biases[MAX_NEURONS];
extern int inputSizes[MAX_NEURONS];

bool isNeuronEqual(void* av, void* bv);
void setNeuronID(void* av, void* bv);
void setNeuronInfo(void* av, void* bv);


void configureNeuron(int neuronId, int receivedInputSize, float* receivedWeights, float receivedBias, int* receivedOrder);
int getNeuronStorageIndex(int neuronId);
int getInputStorageIndex(int neuronId, int inputId);
int getInputSize(int neuronId);
void setInput(int neuronId,float inputValue, int sourceNodeId);
float computeNeuronOutput(int neuronId);
void freeAllNeuronMemory();

#endif //NEURAL_NETWORK_H
