#ifndef NEURAL_NETWORK_MANAGER_H
#define NEURAL_NETWORK_MANAGER_H

#include <cstdio>
#include <cmath>
#include "../message_types.h"
#include "nn_parameters.h"
#include "../nn_configurations.h"
#include "routing.h"
#include "messages.h"


extern TableInfo* neuronToNodeTable;

/*** NeuronId is defined as a uint8_t, allowing up to 256 neurons with IDs ranging from 0 to 255.
    To support a larger neural network (more than 256 neurons), simply change the type of this variable
    to uint16_t, uint32_t, or uint64_t as needed — the rest of the code will continue to function correctly.***/
typedef uint8_t NeuronId;

typedef struct NeuronEntry{
    uint8_t nodeIP[4];
    uint8_t layer;
    uint8_t indexInLayer;
}NeuronEntry;


bool isIDEqual(void* av, void* bv);
void setNeuronId(void* av, void* bv);
void setID(void* av, void* bv);
void setNeuronEntry(void* av, void* bv);
void printNeuronEntry(TableEntry* Table);
void printNeuronTableHeader();

void initNeuralNetwork();

void distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes);
void assignOutputTargetsToNetwork(uint8_t nodes[][4],uint8_t nrNodes);
void assignOutputTargetsToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]);
void assignPubSubInfoToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]);


void encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type);
int encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize,uint8_t neuronId, uint8_t inputSize, uint8_t * inputSaveOrder,const float*weightsValues, float bias);
void encodeAssignOutputMessage(char* messageBuffer, size_t bufferSize, uint8_t * outputNeuronIds, uint8_t nNeurons, uint8_t IPs[][4], uint8_t nNodes);
void encodePubSubInfo(char* messageBuffer, size_t bufferSize, uint8_t * neuronIds, uint8_t nNeurons, uint8_t subTopic, uint8_t pubTopic);

bool isIPinList(uint8_t *searchIP,uint8_t list[][4],uint8_t nElements);

#endif //NEURAL_NETWORK_MANAGER_H
