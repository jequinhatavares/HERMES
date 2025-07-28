#ifndef NEURAL_NETWORK_MANAGER_H
#define NEURAL_NETWORK_MANAGER_H


#include "../nn_types.h"
#include "nn_parameters.h"
#include "../nn_configurations.h"
#include "../worker/neuron_manager.h"
#include "../app_globals.h"

#include <cstdio>
#include <cmath>

extern TableInfo* neuronToNodeTable;


typedef struct NeuronEntry{
    uint8_t nodeIP[4];
    uint8_t layer;
    uint8_t indexInLayer;
    bool isAcknowledged = false;
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
void distributeInputNeurons(uint8_t nodes[][4],uint8_t nrNodes);

void encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type);
int  encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize,uint8_t neuronId, uint8_t inputSize, uint8_t * inputSaveOrder,const float*weightsValues, float bias);
void encodeAssignOutputMessage(char* messageBuffer, size_t bufferSize, uint8_t * outputNeuronIds, uint8_t nNeurons, uint8_t IPs[][4], uint8_t nNodes);
void encodePubSubInfo(char* messageBuffer, size_t bufferSize, uint8_t * neuronIds, uint8_t nNeurons, int8_t subTopic, int8_t pubTopic);
void encodeForwardMessage(char*messageBuffer, size_t bufferSize, int inferenceId);
void encodeInputAssignMessage(char*messageBuffer, size_t bufferSize,uint8_t neuronId);

void handleACKMessage(char* messageBuffer);
void handleWorkerRegistration(char*messageBuffer);

void manageNeuralNetwork();
void onACKTimeOut(uint8_t nodeIP[][4],uint8_t nDevices);
void onACKTimeOutInputLayer();

#endif //NEURAL_NETWORK_MANAGER_H
