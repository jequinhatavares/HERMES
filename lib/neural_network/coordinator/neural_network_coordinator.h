#ifndef NEURAL_NETWORK_MANAGER_H
#define NEURAL_NETWORK_MANAGER_H

#include "nn_parameters.h"
#include "../nn_types.h"
#include "../nn_configurations.h"
#include "../worker/neuron_core.h"
#include "worker/neuron_manager.h"
#include "../app_globals.h"
#include "../nn_monitoring/nn_monitoring.h"

#include <cstdio>
#include <cmath>

extern TableInfo* neuronToNodeTable;


typedef struct NeuronEntry{
    uint8_t nodeIP[4];
    uint8_t layer;
    uint8_t indexInLayer;
    bool isAcknowledged = false;
}NeuronEntry;

class NeuralNetworkCoordinator : public NeuronWorker {
public:
    NeuralNetworkCoordinator();
    ~NeuralNetworkCoordinator();

    void handleNeuralNetworkMessage(uint8_t*senderIP,uint8_t*destinationIP,char* messageBuffer);

    void distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes);
    void distributeNeuralNetworkBalanced(const NeuralNetwork *net, uint8_t devices[][4],uint8_t nrDevices,uint8_t neuronsPerDevice[]);
    void distributeNeuralNetworkBalancedV2(const NeuralNetwork *net,uint8_t devices[][4],uint8_t nrDevices,uint8_t neuronsPerDevice[]);
    void assignOutputTargetsToNetwork(uint8_t nodes[][4],uint8_t nrNodes);
    void assignOutputTargetsToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]);
    void assignOutputTargetsToNeurons(char* messageBuffer,size_t bufferSize,NeuronId *neuronIDs,uint8_t nNeurons,uint8_t targetNodeIP[4]);
    void assignPubSubInfoToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]);
    void distributeInputNeurons(uint8_t nodes[][4],uint8_t nrNodes);
    void distributeOutputNeurons(const NeuralNetwork *net,uint8_t outputDevice[4]);

    static int encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type);
    static int  encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize,uint8_t neuronId, uint8_t inputSize, uint8_t * inputSaveOrder,const float*weightsValues, float bias);
    static void encodeAssignOutputTargetsMessage(char* messageBuffer, size_t bufferSize, uint8_t * outputNeuronIds, uint8_t nNeurons, uint8_t IPs[][4], uint8_t nNodes);
    static void encodePubSubInfo(char* messageBuffer, size_t bufferSize, uint8_t * neuronIds, uint8_t nNeurons, int8_t subTopic, int8_t pubTopic);
    static void encodeForwardMessage(char*messageBuffer, size_t bufferSize, int inferenceId);
    static void encodeInputAssignMessage(char*messageBuffer, size_t bufferSize,uint8_t neuronId);

    void onACKTimeOut(uint8_t nodeIP[][4],uint8_t nDevices);
    void onACKTimeOutInputLayer();

    void manageNeuralNetwork();

private:
    unsigned long neuronAssignmentTime;
    bool areNeuronsAssigned = false;

    bool receivedAllNeuronAcks = false;

    bool inferenceRunning = false;
    unsigned long inferenceStartTime;
    bool inferenceComplete = false;

    int nnSequenceNumber=0;

    // Output layer state management for neural network inference
    // Note: All arrays are indexed by output neuron position (0 to nOutputNeurons-1)
    bool* isOutputReceived;    // Flag array indicating which output neurons have received values
    float* outputNeuronValues; // Buffer containing computed values for output neurons
    int nOutputNeurons=0; //Number of output neurons

    uint8_t workersIPs[TOTAL_NEURONS][4];
    uint8_t workersDeviceTypes[TOTAL_NEURONS];
    uint8_t neuronsPerWorker[TOTAL_NEURONS];
    uint8_t totalWorkers=0;

    uint8_t inputsIPs[TOTAL_INPUT_NEURONS][4];
    uint8_t totalInputs=0;

    uint8_t outputIPs[1][4];
    uint8_t nrOutputDevices=0;


    void initNeuralNetwork();

    void handleACKMessage(char* messageBuffer);
    void handleWorkerRegistration(char*messageBuffer);
    void handleInputRegistration(char* messageBuffer);
    void handleOutputRegistration(char* messageBuffer);

    void assignPubSubInfoToNeuron(char *messageBuffer, size_t bufferSize, NeuronId neuronId);

    bool isOutputNeuron(NeuronId);
    void clearInferenceVariables();

protected:
    //Override the definition of what to do when a NN output is calculated
    void onNeuralNetworkOutput(NeuronId neuronId,float outputValue) override;
};

bool isIDEqual(void* av, void* bv);
void setNeuronId(void* av, void* bv);
void setID(void* av, void* bv);
void setNeuronEntry(void* av, void* bv);
void printNeuronEntry(TableEntry* Table);
void printNeuronTableHeader();



#endif //NEURAL_NETWORK_MANAGER_H
