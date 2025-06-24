#ifndef NEURAL_NETWORK_MANAGER_H
#define NEURAL_NETWORK_MANAGER_H

#include <cstdio>
#include <cmath>
#include "../message_types.h"
#include "nn_parameters.h"
#include "../nn_configurations.h"
#include "routing.h"
#include "messages.h"



void encodeAssignComputationMessage(char* messageBuffer, size_t bufferSize,uint32_t neuronId, uint32_t inputSize, uint32_t * inputSaveOrder,const float*weightsValues, float bias);

#endif //NEURAL_NETWORK_MANAGER_H
