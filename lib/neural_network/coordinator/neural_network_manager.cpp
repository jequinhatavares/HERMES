#include "neural_network_manager.h"
#define NEURAL_NET_IMPL


void distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes){
    uint32_t neuronPerNodeCount = 0,*inputIndexMap;
    int assignedDevices = 0;
    uint32_t numHiddenNeurons =0, neuronsPerNode;
    // Initialize the neuron ID to the first neuron in the first hidden layer (i.e., the first ID after the last input neuron)
    uint32_t currentNeuronId= net->layers[0].numInputs;

    // Count the neurons in the hidden layers, as only these will be assigned to nodes in the network
    for (int i = 0; i < net->numHiddenLayers; i++) {
        numHiddenNeurons += net->layers[i].numOutputs;
    }

    // Calculate how many neurons will be assigned to each node
    neuronsPerNode = ceil( numHiddenNeurons / nrNodes);

    LOG(APP, INFO, "Neural network initialized with %d neurons (avg. %d neurons per node)\n", net->numNeurons, neuronsPerNode);

    for (int i = 0; i < net->numLayers; i++) { //For each hidden layer

        /***Initialize the input index mapping before processing each layer. The inputIndexMapping specifies the order
         in which the node should store input values. It corresponds to an ordered list of neuron IDs from the previous
         layer, since those neurons serve as inputs to the current layer.***/
        inputIndexMap = new uint32_t[net->layers[i].numInputs];
        for (int j = 0; j < net->layers[i].numInputs ; j++){
            // For the first hidden layer, the input index mapping corresponds to the neuron IDs of the input layer (ranging from 0 to nrInputs - 1).
            //if(i == 0)inputIndexMap[j] = j;
            // For subsequent hidden layers range from (currentNeuronId - (neuronsInPreviousLayer)) to (currentNeuronId)
            inputIndexMap[j] = currentNeuronId+(j-net->layers[i].numInputs);
        }

        // The root node is responsible for computing the output layer.
        if(i == net->numLayers - 1){
            //TODO this node computed the output layer
            LOG(APP, INFO, "Layer %i reserved for root calculation\n",net->numHiddenLayers-1);
            continue;
        }

        for (int j = 0; j < net->layers[i].numOutputs; j++) { // For each neuron in each layer

            LOG(APP, INFO, "Node IP: %i.%i.%i.%i\n",nodes[assignedDevices][0],nodes[assignedDevices][1],nodes[assignedDevices][2],nodes[assignedDevices][3]);

            encodeAssignComputationMessage(largeSendBuffer, sizeof(largeSendBuffer),
                                           currentNeuronId,net->layers[i].numInputs,inputIndexMap,
                                           &net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j]);

            // Increment the count of neurons assigned to this node, and the current NeuronID
            currentNeuronId ++;
            neuronPerNodeCount++;

            // When the number of neurons assigned to the current device reaches the expected amount, move on to the next device in the list.
            if(neuronPerNodeCount == neuronsPerNode){
                assignedDevices ++;
                neuronPerNodeCount = 0;

                if(assignedDevices>=nrNodes){
                    LOG(APP, ERROR, "ERROR: The number of assigned devices exceeded the total available nodes for neuron assignment.\n");
                    return;
                }
            }
        }

        delete [] inputIndexMap;
    }

}

void encodeAssignComputationMessage(char* messageBuffer, size_t bufferSize, uint32_t neuronId, uint32_t inputSize, uint32_t * inputSaveOrder, const float* weightsValues, float bias){
    //NN_ASSIGN_COMPUTATION [Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]
    int offset = 0;

    //Encode: NN_ASSIGN_COMPUTATION [Neuron Number 1] [Input Size]
    offset = snprintf(messageBuffer,bufferSize,"%i %i %i ",NN_ASSIGN_COMPUTATION,neuronId,inputSize);

    //Encode the [Input Save Order] vector
    for (int i = 0; i < inputSize; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%i ",inputSaveOrder[i]);
    }

    //Encode the [weights values] vector
    for (int i = 0; i < inputSize; i++) {
        //%g format specifier automatically chooses the most compact representation, removing unnecessary trailing zeros
        //2.0 -> 2 , 2.1->2.1
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%g ",weightsValues[i]);
    }

    offset += snprintf(messageBuffer + offset, bufferSize - offset,"%g",bias);
    LOG(APP, INFO, "Encoded Message: NN_ASSIGN_COMPUTATION [Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]\n");
    LOG(APP, INFO, "Encoded Message: %s\n",messageBuffer);
}