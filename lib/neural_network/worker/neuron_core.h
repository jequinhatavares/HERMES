#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <cstring>
//#include <logger.h>
#include "../nn_configurations.h"
#include "../nn_types.h"

#include "../app_globals.h"

class NeuronCore {

public:
    int neuronsCount = 0;               // Number of neurons currently computed by this node

    // Constructor / Destructor
    NeuronCore();
    ~NeuronCore();

    void configureNeuron(NeuronId neuronId, uint8_t receivedInputSize, float* receivedWeights, float receivedBias, NeuronId* receivedOrder);

    NeuronId getNeuronId(int index);
    int getNeuronStorageIndex(NeuronId neuronId);
    int getInputStorageIndex(NeuronId neuronId, NeuronId inputId);
    int getInputSize(NeuronId neuronId);

    NeuronId getInputNeuronId(NeuronId neuronId, int index);
    float getNeuronWeightAtIndex(NeuronId neuronId,int index);
    float getBias(NeuronId neuronId);
    float getNeuronInputValue(NeuronId neuronId, NeuronId inputNeuronId);

    bool isInputRequired(NeuronId neuronId,NeuronId inputId);
    bool computesNeuron(NeuronId neuronId);

    void setInput(NeuronId neuronId,float inputValue, NeuronId sourceNodeId);
    float computeNeuronOutput(NeuronId neuronId);

    void freeAllNeuronMemory();

    void printNeuronInfo();

private:
    NeuronId neuronIds[MAX_NEURONS];    // Each node can compute until MAX_NEURONS each one with a node Id
    float* weights[MAX_NEURONS];        // Each neuron has its own weight array allocated at weights[NeuronStorageIndex]
    float* inputs[MAX_NEURONS];         // Each neuron has its own input buffer allocated at weights[NeuronStorageIndex]
    NeuronId* saveOrders[MAX_NEURONS];  // Each neuron has its own save order allocated at weights[NeuronStorageIndex]
    float biases[MAX_NEURONS];          // Each neuron has its own bias allocated at weights[NeuronStorageIndex]
    uint8_t inputSizes[MAX_NEURONS];    // Each neuron has its own input size allocated at weights[NeuronStorageIndex]

};


bool isNeuronInList(NeuronId *neuronsList, uint8_t nNeurons, NeuronId targetNeuronId);








#endif //NEURAL_NETWORK_H
