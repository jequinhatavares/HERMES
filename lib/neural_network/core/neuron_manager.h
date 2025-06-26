#ifndef NEURON_MANAGER_H
#define NEURON_MANAGER_H

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include "neuron_core.h"
#include "../message_types.h"


typedef uint32_t BitField;

extern float outputValue;

extern BitField receivedInputs[MAX_NEURONS];



void handleNeuralNetworkMessage(char* messageBuffer);
void handleNeuronInput(int neuronId,int outputNeuronId,float inputValue);

/**
 * setBit
 * Sets the i-th bit (0-indexed from LSB) in the bit field
 *
 * @param bits - Reference to bit field to modify
 * @param i    - Bit position to set (0-indexed)
 **/
inline void setBit(BitField& bits, uint8_t i) {
    bits |= (1U << i); //1U is the unsigned integer value 1-> 0b00000001
}

/**
 * countBits
 * Counts the number of set bits in the bit field
 *
 * @param bits - Bit field to examine
 * @return Number of bits set to 1
 **/
inline uint8_t countBits(BitField bits) {
    return __builtin_popcount(bits); // GCC/Clang intrinsic
}

/**
 * allBits
 * Checks if the lowest n bits are all set in the bit field
 *
 * @param bits - Bit field to check
 * @param n    - Number of lowest bits to verify
 * @return True if all n bits are set, false otherwise
 **/
inline bool allBits(BitField bits, uint8_t n) {
    return bits == ((1U << n) - 1);// Check if the lowest 'n' bits are all set to 1 (i.e., bits == 2^n - 1)
}

/**
 * resetAll
 * Clears all bits in the bit field (sets to 0)
 *
 * @param bits - Reference to bit field to reset
 **/
inline void resetAll(BitField& bits){
    bits = 0;
}

#endif //NEURON_MANAGER_H
