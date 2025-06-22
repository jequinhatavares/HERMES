#ifndef NEURON_MANAGER_H
#define NEURON_MANAGER_H

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include "neuron_core.h"
#include "../message_types.h"


typedef uint32_t BitField;


void handleNeuralNetworkMessage(char* messageBuffer);
void handleNeuronInput(int neuronId,int outputNeuronId);

inline void setBit(BitField& bits, uint8_t i);
inline uint8_t countBits(BitField bits);
inline bool allBits(BitField bits, uint8_t n);
inline void resetAll(BitField& bits);

#endif //NEURON_MANAGER_H
