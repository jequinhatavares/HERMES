#include "neuron_core.h"

NeuronCore::NeuronCore() {

}

NeuronCore::~NeuronCore() {
    freeAllNeuronMemory();
}

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
void NeuronCore::configureNeuron(NeuronId neuronID, uint8_t receivedInputSize, float* receivedWeights, float receivedBias, NeuronId* receivedOrder) {
    if (neuronsCount >= MAX_NEURONS) {
        LOG(APP,ERROR, "ERROR: Exceeded max neurons per node.\n");
        return;
    }

    //Add the neuronId to the list of neurons computed by this node
    neuronIds[neuronsCount] = neuronID;
    //Add the inputSize of this neuron to the list
    inputSizes[neuronsCount] = receivedInputSize;

    //Append the weights values to the list
    weights[neuronsCount] = new float[receivedInputSize];
    memcpy(weights[neuronsCount], receivedWeights, sizeof(float) * receivedInputSize);

    //Initialize the pointer to the memory where the inputs will be stored
    inputs[neuronsCount] = new float[receivedInputSize](); // initialized with zeros

    //Append the saveInput order to the list
    saveOrders[neuronsCount] = new NeuronId[receivedInputSize];
    memcpy(saveOrders[neuronsCount], receivedOrder, sizeof(NeuronId) * receivedInputSize);

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
int NeuronCore::getNeuronStorageIndex(NeuronId neuronID){
    int neuronStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    for (int i = 0; i < neuronsCount; i++) {
        if(neuronID == neuronIds[i]) neuronStorageIndex=i;
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
int NeuronCore::getInputStorageIndex(NeuronId neuronId, NeuronId inputId){
    int neuronStorageIndex = -1, inputStorageIndex = -1;

    // Find the index in the local vectors (weights, inputs, saveOrder) where the neuron's parameters were stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    if(neuronStorageIndex == -1) return -1;

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
int NeuronCore::getInputSize(NeuronId neuronId){
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
bool NeuronCore::isInputRequired(NeuronId neuronId,NeuronId inputId){
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
 * computesNeuron
 * Checks if a specific neuron is handled/computed by this node
 *
 * @param neuronId The ID of the neuron to check
 * @return true If the neuron is handled by this node
 *         false otherwise
 */
bool NeuronCore::computesNeuron(NeuronId neuronId){
    return(getNeuronStorageIndex(neuronId) != -1);
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
void NeuronCore::setInput(NeuronId neuronId, float inputValue, NeuronId sourceNodeId){
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
float NeuronCore::computeNeuronOutput(NeuronId neuronId){
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
void NeuronCore::freeAllNeuronMemory() {
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

    }
    //Reset the number of neurons computed by this node to zero
    neuronsCount = 0;
}

//todo header
bool isNeuronInList(NeuronId *neuronsList, uint8_t nNeurons, NeuronId targetNeuronId){
    for (uint8_t i = 0; i < nNeurons; i++) {
        if(neuronsList[i] == targetNeuronId) return true;
    }
    return false;
}

void NeuronCore::printNeuronInfo() {
    LOG(APP, INFO, "====================== Computed Neurons (%d total) ======================\n", neuronsCount);

    for (int i = 0; i < neuronsCount; ++i) {
        char line[512];
        int offset = 0;

        // Start the line with metadata
        offset += snprintf(line + offset, sizeof(line) - offset,
                           "Neuron %d | ID: %u | Bias: %.4f | InputSize: %u | Weights: [",
                           i, neuronIds[i], biases[i], inputSizes[i]);

        // Append weight values
        for (int j = 0; j < inputSizes[i]; ++j) {
            offset += snprintf(line + offset, sizeof(line) - offset, "%.4f", weights[i][j]);
            if (j < inputSizes[i] - 1) offset += snprintf(line + offset, sizeof(line) - offset, ",");
        }

        // Append save order values
        offset += snprintf(line + offset, sizeof(line) - offset, "] | SaveOrder: [");
        for (int j = 0; j < inputSizes[i]; ++j) {
            offset += snprintf(line + offset, sizeof(line) - offset, "%u", saveOrders[i][j]);
            if (j < inputSizes[i] - 1) offset += snprintf(line + offset, sizeof(line) - offset, ",");
        }

        // Append input values
        offset += snprintf(line + offset, sizeof(line) - offset, "] | Gathered inputs: [");
        for (int j = 0; j < inputSizes[i]; ++j) {
            offset += snprintf(line + offset, sizeof(line) - offset, "%.4f", inputs[i][j]);
            if (j < inputSizes[i] - 1) offset += snprintf(line + offset, sizeof(line) - offset, ",");
        }

        // Close the line
        offset += snprintf(line + offset, sizeof(line) - offset, "]");

        // Finally log the full line
        LOG(APP, INFO, "%s\n", line);
    }

    LOG(APP, INFO, "===================================\n");
}


