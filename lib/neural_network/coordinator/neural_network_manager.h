#ifndef NEURAL_NETWORK_MANAGER_H
#define NEURAL_NETWORK_MANAGER_H

#include <cstdio>
#include <cmath>
#include "../message_types.h"
#include "nn_parameters.h"
#include "../nn_configurations.h"
#include "routing.h"
#include "messages.h"

typedef struct NeuronMap{
    uint8_t nodeIP[4];
    uint8_t layer;
    uint8_t indexInLayer;
}NeuronMap;

bool isIDEqual(void* av, void* bv);
void setNeuronId(void* av, void* bv);
void setID(void* av, void* bv);
void setNeuronMap(void* av, void* bv);

void initNeuralNetwork();

void distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes);

int encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type);
int encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize,uint32_t neuronId, uint32_t inputSize, uint32_t * inputSaveOrder,const float*weightsValues, float bias);
void encodeAssignOutputMessage(char* messageBuffer, size_t bufferSize, int* outputNeuronIds, int nNeurons, uint8_t IPs[][4], uint8_t nNodes);


#endif //NEURAL_NETWORK_MANAGER_H
