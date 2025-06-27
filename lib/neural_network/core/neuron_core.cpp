#include "neuron_core.h"


int neuronIds[MAX_NEURONS];
float* weights[MAX_NEURONS];     // Each neuron has its own weight array allocated at weights[NeuronStorageIndex]
float* inputs[MAX_NEURONS];      // Each neuron has its own input buffer allocated at weights[NeuronStorageIndex]
int* saveOrders[MAX_NEURONS];    // Each neuron has its own save order allocated at weights[NeuronStorageIndex]
float biases[MAX_NEURONS];       // Each neuron has its own bias allocated at weights[NeuronStorageIndex]
int inputSizes[MAX_NEURONS];     // Each neuron has its own input size allocated at weights[NeuronStorageIndex]

int neuronsCount = 0;             //Number of neurons currently computed by this node


/**
 * configureNeuron
  * Initializes memory and saves the configuration for a neuron that this node is responsible for computing.
 * This includes storing its input size, weights, bias, and the input save order mapping.
 *
 * @param neuronId           Unique identifier of the neuron within the distributed neural network.
 * @param receivedInputSize  Number of inputs this neuron will receive (i.e., its number of weights).
 * @param receivedWeights    Pointer to the array containing the neuron's weights.
 * @param receivedBias       Bias value to be used during the neuron's output computation.
 * @param receivedOrder      Pointer to the array specifying the input save order (mapping where each received input
 * should be stored in the input vector).
 * @return void
 */
void configureNeuron(int neuronId, int receivedInputSize, float* receivedWeights, float receivedBias, int* receivedOrder) {
    if (neuronsCount >= MAX_NEURONS) {
        LOG(APP,ERROR, "ERROR: Exceeded max neurons per node.\n");
        return;
    }

    //Add the neuronId to the list of neurons computed by this node
    neuronIds[neuronsCount] = neuronId;
    //Add the inputSize of this neuron to the list
    inputSizes[neuronsCount] = receivedInputSize;

    //Append the weights values to the list
    weights[neuronsCount] = new float[receivedInputSize];
    memcpy(weights[neuronsCount], receivedWeights, sizeof(float) * receivedInputSize);

    //Initialize the pointer to the memory where the inputs will be stored
    inputs[neuronsCount] = new float[receivedInputSize]; // uninitialized, filled during inference

    //Append the saveInput order to the list
    saveOrders[neuronsCount] = new int[receivedInputSize];
    memcpy(saveOrders[neuronsCount], receivedOrder, sizeof(int) * receivedInputSize);

    //Initialize the node bias value
    biases[neuronsCount] = receivedBias;

    //Increment the number of computed neurons
    neuronsCount++;
}

/**
 * getNeuronStorageIndex
 * Searches for the specified neuron ID among the neurons managed by this node and returns
 * the corresponding index used to access its stored parameters (weights, inputs, etc.).
 *
 * @param neuronId - Unique identifier of the neuron to locate
 * @return Storage index if found, -1 if neuron not managed by this node
 */
int getNeuronStorageIndex(int neuronId){
    int neuronStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    for (int i = 0; i < neuronsCount; i++) {
        if(neuronId == neuronIds[i]) neuronStorageIndex=i;
    }
    return neuronStorageIndex;
}

/**
 * getInputStorageIndex
 * Locates the storage position for a specific input value within a neuron's input vector
 *
 * @param neuronId - Identifier of the target neuron
 * @param inputId  - Identifier of the neuron that provided the input value.
 * @return Index within the neuron's input vector where this input should be stored,
 *         or -1 if the neuron is not managed by this node or if inputId is not found.
 */
int getInputStorageIndex(int neuronId, int inputId){
    int neuronStorageIndex = -1, inputStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    // Locate the index in the inputs vector where the new input should be stored, as specified by the saveOrder vector
    for (int i = 0; i < inputSizes[neuronStorageIndex]; i++) {
        if(saveOrders[neuronStorageIndex][i]==inputId){
            inputStorageIndex = i;
        }
    }
    return inputStorageIndex;
}

/**
 * getInputSize
 * Returns the number of inputs for the specified neuron if managed by this node.
 *
 * @param neuronId - Identifier of the target neuron
 * @return Number of inputs if neuron found, -1 if neuron not managed by this node
 */
int getInputSize(int neuronId){
    int neuronStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    if(neuronStorageIndex == -1) return -1;

    return inputSizes[neuronStorageIndex];
}

/**
 * isInputRequired
 * Checks if a specific input is required by a neuron
 *
 * @param neuronId The ID of the neuron to check
 * @param inputId The ID of the input to verify
 * @return true If the neuron requires the specified input
 *         false If the neuron doesn't require the input or doesn't exist
 */
bool isInputRequired(int neuronId,int inputId){
    int neuronStorageIndex = -1, inputSize = 0;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    if(neuronStorageIndex == -1) return false;

    // Check if the input ID exists in the saveOrder list
    inputSize = inputSizes[neuronStorageIndex];
    for(int i = 0; i < inputSize; i++) {
        if(saveOrders[neuronStorageIndex][i] == inputId) return true;
    }

    return false;
}


/**
 * setInput
 * Stores an input value for a specific neuron at the correct buffer position
 *
 * @param neuronId - Identifier of the target neuron
 * @param inputValue - Value to be stored in the input buffer
 * @param sourceNodeId - ID of the node that generated the corresponding neuron output
 * @return void
 */
void setInput(int neuronId, float inputValue, int sourceNodeId){
    int inputStorageIndex = -1, neuronStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    // Return if the neuron ID is not among those computed by this node
    if(neuronStorageIndex == -1){
        LOG(APP,ERROR, "ERROR: Neuron ID not found in the list of neurons computed by this node\n");
        return;
    }

    // Locate the index in the inputs vector where the new input should be stored, as specified by the saveOrder vector
    inputStorageIndex = getInputStorageIndex(neuronId, sourceNodeId);

    if(inputStorageIndex != -1){
        inputs[neuronStorageIndex][inputStorageIndex] = inputValue;
        //LOG(APP,DEBUG,"input[%i][%i] = %f\n",neuronStorageIndex,inputStorageIndex,inputValue);
    }
}

/**
 * ReLu
 * Computes the Rectified Linear Unit activation function output
 *
 * @param x - Input value to the activation function
 * @return x if x > 0, otherwise 0
 */
float ReLu(float x){
    return x > 0 ? x : 0;
}

/**
 * computeNeuronOutput
 * Computes the output of a given neuron by applying a weighted sum of its inputs
 * followed by an activation function.
 *
 * @param neuronId - Unique identifier of the neuron to compute
 * @return The computed output value of the neuron, or -99999.9 if the neuron is not found
 */
float computeNeuronOutput(int neuronId){
    int neuronStorageIndex = -1;
    float sum = 0;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    // Return if the neuron ID is not among those computed by this node
    if(neuronStorageIndex == -1){
        LOG(APP,ERROR, "ERROR: Unable to compute neuron output: neuron ID not found among those handled by this node\n");
        return -99999.9;
    }

    //First add the node bias to the sum
    sum += biases[neuronStorageIndex];

    //Then multiply each input by each corresponding weight
    for (int i = 0; i < inputSizes[neuronStorageIndex]; i++) {
        //LOG(APP,DEBUG,"input:%f weights:%f\n",inputs[neuronStorageIndex][i],weights[neuronStorageIndex][i]);
        sum += inputs[neuronStorageIndex][i] * weights[neuronStorageIndex][i];
    }

    //LOG(APP,DEBUG,"final value:%f\n",sum);

    //Finally pass the sum result by the activation function
    return ReLu(sum);  // Activation function
}

/**
 * freeAllNeuronMemory
 * Releases all dynamically allocated memory for the neurons parameters and resets state
 *
 * @return void
 */
void freeAllNeuronMemory() {
    for (int i = 0; i < neuronsCount; ++i) {
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
        neuronsCount = 0;
    }
}


