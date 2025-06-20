#include "neuron_core.h"


int neuronIds[MAX_NEURONS];
float* weights[MAX_NEURONS];     // Each neuron has its own weight array allocated at weights[NeuronStorageIndex]
float* inputs[MAX_NEURONS];      // Each neuron has its own input buffer allocated at weights[NeuronStorageIndex]
int* saveOrders[MAX_NEURONS];    // Each neuron has its own save order allocated at weights[NeuronStorageIndex]
float biases[MAX_NEURONS];       // Each neuron has its own bias allocated at weights[NeuronStorageIndex]
int inputSizes[MAX_NEURONS];     // Each neuron has its own input size allocated at weights[NeuronStorageIndex]

int neuronCount = 0;             //Number of neurons currently computed by this node



bool isNeuronEqual(void* av, void* bv) {
    int *a = (int*) av;
    int *b = (int*) bv;
    return(*a==*b);
}

void setNeuronID(void* av, void* bv){
    int *a = (int*) av;
    int *b = (int*) bv;
    *a=*b;
}

void setNeuronInfo(void* av, void* bv){
    NeuronInfo *a = (NeuronInfo*) av;
    NeuronInfo *b = (NeuronInfo*) bv;
    a->saveOrder = b->saveOrder;
    a->weights = b->weights;
    a->inputs = b->inputs;
    a->bias = b->bias;
    a->inputSize = b->inputSize;
}


void configureNeuron(int neuronId, int receivedInputSize, float* receivedWeights, float receivedBias, int* receivedOrder) {
    if (neuronCount >= MAX_NEURONS) {
        LOG(APP,ERROR, "ERROR: Exceeded max neurons per node.");
        return;
    }

    //Add the neuronId to the list of neurons computed by this node
    neuronIds[neuronCount] = neuronId;
    //Add the inputSize of this neuron to the list
    inputSizes[neuronCount] = receivedInputSize;

    //Append the weights values to the list
    weights[neuronCount] = new float[receivedInputSize];
    memcpy(weights[neuronCount], receivedWeights, sizeof(float) * receivedInputSize);

    //Initialize the pointer to the memory where the inputs will be stored
    inputs[neuronCount] = new float[receivedInputSize]; // uninitialized, filled during inference

    //Append the saveInput order to the list
    saveOrders[neuronCount] = new int[receivedInputSize];
    memcpy(saveOrders[neuronCount], receivedOrder, sizeof(int) * receivedInputSize);

    //Initialize the node bias value
    biases[neuronCount] = receivedBias;

    //Increment the number of computed neurons
    neuronCount++;
}

int getNeuronStorageIndex(int neuronId){
    int neuronStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    for (int i = 0; i < neuronCount; i++) {
        if(neuronId == neuronIds[i]) neuronStorageIndex=i;
    }
    return neuronStorageIndex;
}

void setInput(int neuronId, float inputValue, int inputID){
    int inputStorageIndex = -1, neuronStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    // Return if the neuron ID is not among those computed by this node
    if(neuronStorageIndex == -1){
        LOG(APP,ERROR, "ERROR: Neuron ID not found in the list of neurons computed by this node");
        return;
    }

    // Locate the index in the inputs vector where the new input should be stored, as specified by the saveOrder vector
    for (int i = 0; i < inputSizes[neuronStorageIndex]; i++) {
        if(saveOrders[neuronStorageIndex][i]==inputID){
            inputStorageIndex = i;
        }
    }

    LOG(NETWORK,DEBUG,"save index:%i\n",inputStorageIndex);
    if(inputStorageIndex != -1){
        inputs[neuronStorageIndex][inputStorageIndex] = inputValue;
    }
}

float ReLu(float x){
    return x > 0 ? x : 0;
}


float computeNeuronOutput(int neuronId){
    int neuronStorageIndex = -1;
    float sum = 0;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    // Return if the neuron ID is not among those computed by this node
    if(neuronStorageIndex == -1){
        LOG(APP,ERROR, "ERROR: Unable to compute neuron output: neuron ID not found among those handled by this node");
        return -99999.9;
    }

    //First add the node bias to the sum
    sum += biases[neuronStorageIndex];

    //Then multiply each input by each corresponding weight
    for (int i = 0; i < inputSizes[neuronStorageIndex]; i++) {
        sum += inputs[neuronStorageIndex][i] * weights[neuronStorageIndex][i];
    }

    //Finally pass the sum result by the activation function
    return ReLu(sum);  // Activation function
}

void freeAllNeuronMemory() {
    for (int i = 0; i < neuronCount; ++i) {
        delete[] weights[i];
        delete[] inputs[i];
        delete[] saveOrders[i];

        weights[i] = nullptr;
        inputs[i] = nullptr;
        saveOrders[i] = nullptr;

        // Reset metadata
        neuronIds[i] = -1;
        biases[i] = 0.0f;
        inputSizes[i] = 0;

        //Reset the number of neurons computed by this node to zero
        neuronCount = 0;
    }
}


