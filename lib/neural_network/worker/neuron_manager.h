#ifndef NEURON_MANAGER_H
#define NEURON_MANAGER_H

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include "../nn_types.h"
#include "neuron_core.h"
#include "../app_globals.h"


typedef uint32_t BitField;

extern float outputValues[MAX_NEURONS];


/***typedef struct OutputTarget{
    uint8_t (*outputTargets)[4];  // pointer to array of 4-byte IPs
    uint8_t nTargets;
}OutputTarget;***/

typedef struct OutputTarget{
    uint8_t targetsIPs[MAX_TARGET_OUTPUTS][4];
    uint8_t nTargets = 0;
}OutputTarget;

extern BitField receivedInputs[MAX_NEURONS];

extern OutputTarget neuronTargets[MAX_NEURONS];

extern bool isOutputComputed[MAX_NEURONS];

// Contains the input neurons that this node hosts
extern NeuronId inputNeurons[MAX_INPUT_NEURONS];

extern OutputTarget inputTargets;

extern float inputNeuronsValues[MAX_INPUT_NEURONS];

void handleNeuronMessage(char* messageBuffer);

void handleAssignComputationsMessage(char*messageBuffer);
void handleAssignOutputTargets(char* messageBuffer);
void handleAssignInput(char* messageBuffer);
void handleAssignOutput(char* messageBuffer);
void handleAssignPubSubInfo(char* messageBuffer);
void handleNACKMessage(char*messageBuffer);
void handleNeuronOutputMessage(char*messageBuffer);
void handleForwardMessage(char *messageBuffer);

void updateOutputTargets(uint8_t nNeurons, uint8_t *neuronId, uint8_t targetIP[4]);

void encodeNeuronOutputMessage(char* messageBuffer,size_t bufferSize,NeuronId outputNeuronId, float neuronOutput);
void encodeNACKMessage(char* messageBuffer, size_t bufferSize,NeuronId missingNeuron);
void encodeACKMessage(char* messageBuffer, size_t bufferSize,NeuronId *neuronAckList, int ackNeuronCount);
void encodeWorkerRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4],DeviceType type);

void processNeuronInput(NeuronId outputNeuronId,int inferenceId,float inputValue);
void generateInputData(NeuronId inputNeuronId);
int getInputNeuronStorageIndex(NeuronId neuronId);
void manageNeuron();

void onInputWaitTimeout();
void onNACKTimeout();

void clearAllNeuronMemory();

 //TODO por esta função num sitio melhor

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
 * isBitSet
 * Checks if the specified bit is set in the given bit field.
 *
 * @param bits - The bit field to check
 * @param i - The index of the bit to check
 * @return True if the bit is set, false otherwise
 */
inline bool isBitSet(BitField bits, uint8_t i) {
    return (bits & (1U << i)) != 0;
}

/**
 * allBitsZero
 * Checks if all bits in the given bit field are zero (unset).
 *
 * @param bits - The bit field to check
 * @return True if all bits are zero, false otherwise
 */
inline bool allBitsZero(BitField bits) {
    return bits == 0;
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


