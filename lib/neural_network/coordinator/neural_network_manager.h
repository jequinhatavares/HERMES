#ifndef NEURAL_NETWORK_MANAGER_H
#define NEURAL_NETWORK_MANAGER_H

#include "../message_types.h"
#include <cstdio>


void encodeAssignComputationMessage(char* messageBuffer, size_t bufferSize,int neuronId, int inputSize, int* inputSaveOrder,float*weightsValues, float bias);

#endif //NEURAL_NETWORK_MANAGER_H
