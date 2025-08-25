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

typedef struct OutputTarget{
    uint8_t targetsIPs[MAX_TARGET_OUTPUTS][4];
    uint8_t nTargets = 0;
}OutputTarget;

class NeuronWorker{
public:
    /*** Stores the target nodes of the input neurons. Since all input neurons belong to the same layer,
     * they share the same set of target nodes (i.e., the neurons in the next layer).
     * Therefore, only one instance of the OutputTarget structure is needed. ***/
    OutputTarget inputTargets;

    /*** Each node has a bitfield that indicates which inputs have been received during the current forward propagation
   *  step of the neural network.receivedInputs[neuronIndex][inputIndex] == 1 means the input was received. ***/
    BitField receivedInputs[MAX_NEURONS] = {0};

    // Store the output value of each neuron
    float outputValues[MAX_NEURONS];

    // Indicates whether the output of a given neuron has already been computed
    bool isOutputComputed[MAX_NEURONS]={ false};

    // To store the target nodes of each neuron output
    OutputTarget neuronTargets[MAX_NEURONS];

    // Contains the input neurons that this node hosts
    NeuronId inputNeurons[MAX_INPUT_NEURONS];
    uint8_t nrInputNeurons=0;

    // Contain the input values of the current inference iteration
    float inputNeuronsValues[MAX_INPUT_NEURONS];

    NeuronCore neuronCore; //owns the neuron core data & computations

    void handleNeuronMessage(uint8_t* senderIP,uint8_t* destinationIP,char* messageBuffer);
    void manageNeuron();

    void registerNodeAsInput();
    void registerNodeAsWorker();
    void registerNodeAsOutput();

    static void encodeNeuronOutputMessage(char* messageBuffer,size_t bufferSize,int inferenceId,NeuronId outputNeuronId, float neuronOutput);
    static void encodeNACKMessage(char* messageBuffer, size_t bufferSize,NeuronId missingNeuron);
    static void encodeACKMessage(char* messageBuffer, size_t bufferSize,NeuronId *neuronAckList, int ackNeuronCount);
    static void encodeWorkerRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4],DeviceType type);
    static void encodeInputRegistration(char *messageBuffer, size_t bufferSize, uint8_t *nodeIP, DeviceType type);
    static void encodeOutputRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4]);
    void decodeNeuronTopic(char* dataMessage, int8_t* topicType);

    void clearAllNeuronMemory();

private:
    // Variables used to track when the NACK mechanism was triggered, to manage this neuron's output in the presence of missing inputs
    unsigned long nackTriggerTime;
    bool nackTriggered = false;

    // Timestamp marking the start of the current forward propagation cycle
    unsigned long firstInputTimestamp;
    // Variable to track if a NN forward pass is currently running
    bool forwardPassRunning = false;

    // Indicates whether all outputs of the neurons owned by this node have been computed
    bool allOutputsComputed = false;

    // Identifier of the current inference cycle, assigned by the root node
    int currentInferenceId = 0;

    //List of neurons that this node handles that are output neurons
    NeuronId outputNeurons[MAX_NEURONS];
    uint8_t nrOutputNeurons=0;

    void (*onOutputLayerComputation)(NeuronId outputNeuronId, float outputValue) = nullptr;

    void handleAssignComputationsMessage(char*messageBuffer);
    void handleAssignOutputTargets(char* messageBuffer);
    void handleAssignInput(char* messageBuffer);
    void handleAssignOutputNeuron(char* messageBuffer);
    void handleAssignPubSubInfo(char* messageBuffer);
    void handleNACKMessage(char*messageBuffer,uint8_t*senderIP);
    void handleNeuronOutputMessage(char*messageBuffer);
    void handleForwardMessage(char *messageBuffer);

    void updateOutputTargets(uint8_t nNeurons, uint8_t *neuronId, uint8_t targetIP[4]);

    void processNeuronInput(NeuronId outputNeuronId,int inferenceId,float inputValue);
    void generateInputData(NeuronId inputNeuronId);
    int getInputNeuronStorageIndex(NeuronId neuronId);

    void onInputWaitTimeout();
    void onNACKTimeout();

    void clearNeuronOutputTargets(NeuronId neuronId);

protected:
    // saves the topics that each node publishes
    int8_t neuronToTopicMap[MAX_NEURONS];

    //Functions to be used by the coordinator class to initialize the neuronworker
    // parameters without accessing its variables directly (to be impemented)
    void saveOutputNeuron(NeuronId outputNeuronId);
    void saveInputNeuron(NeuronId inputNeuronId);
    void saveWorkerTargets(NeuronId neuronId,uint8_t targetNodeIP[][4],uint8_t nTargets);
    void saveWorkerPubSubInfo(NeuronId neuronId,int8_t pubTopic);

};

/***typedef struct OutputTarget{
    uint8_t (*outputTargets)[4];  // pointer to array of 4-byte IPs
    uint8_t nTargets;
}OutputTarget;***/


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

bool isTopicInList(int8_t *topicList, int listSize, int8_t searchTopic);
#endif //NEURON_MANAGER_H


