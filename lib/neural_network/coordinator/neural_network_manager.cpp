#include "neural_network_manager.h"

#define NEURAL_NET_IMPL


void distributeNeuralNetwork(const NeuralNetwork *net){
    uint32_t neuronPerNodeCount = 0,currentNeuronId= net->layers[0].numInputs,*inputIndexMap;
    int assignedDevices = 0, nodeIP[4], assignedNeurons = 0;
    int firstNeuronId = 0 ;
    uint32_t numDevices = getNumberOfActiveDevices();
    routingTableEntry *routingEntry;

    uint32_t neuronsPerNode = ceil( net->numNeurons / numDevices);

    LOG(APP, INFO, "Neural network initialized with %d neurons (avg. %d neurons per node)\n", net->numNeurons, neuronsPerNode);

    for (int i = 0; i < net->numHiddenLayers; i++) { //For each hidden layer

        /***Initialize the input index mapping before processing each layer. The inputIndexMapping specifies the order
         in which the node should store input values. It corresponds to an ordered list of neuron IDs from the previous
         layer, since those neurons serve as inputs to the current layer.***/
        inputIndexMap = new uint32_t[net->layers[i].numInputs];
        for (int j = 0; j < net->layers[i].numInputs ; j++){
            // For the first hidden layer, the input index mapping corresponds to the neuron IDs of the input layer (ranging from 0 to nrInputs - 1).
            if(i == 0)inputIndexMap[j] = j;
            // For subsequent hidden layers range from (currentNeuronId - (neuronsInPreviousLayer-1)) to (currentNeuronId
            else inputIndexMap[j] = currentNeuronId+(j-net->layers[i].numInputs+1);
        }

        for (int j = 0; j < net->layers[i].numOutputs; j++) { // For each neuron in each layer
            //Todo isto pode ser uma função no routing se tenho uma route para um nó
            routingEntry = (routingTableEntry*) tableValueAtIndex(routingTable,assignedDevices);
            if(routingEntry != nullptr){
                // Neurons are only assigned to devices with an established route.
                if(routingEntry->hopDistance != -1){
                    encodeAssignComputationMessage(largeSendBuffer, sizeof(largeSendBuffer), currentNeuronId,net->layers[i].numInputs, inputIndexMap,net->layers[i].weights[neuronPerNodeCount],net->layers[i].biases[neuronPerNodeCount]);
                    // Increment the total number of assigned neurons, the count of neurons assigned to this node, and the current NeuronID
                    assignedNeurons ++;
                    neuronPerNodeCount++;
                    currentNeuronId ++;
                }else{
                    j-=1;
                }
            }else{
                j-=1;
            }

            // When the number of neurons assigned to the current device reaches the expected amount, move on to the next device in the list.
            if(neuronPerNodeCount == neuronsPerNode){
                assignedDevices ++;
                neuronPerNodeCount = 0;
            }

        }

        delete [] inputIndexMap;
    }

}

void encodeAssignComputationMessage(char* messageBuffer, size_t bufferSize, uint32_t neuronId, uint32_t inputSize, uint32_t * inputSaveOrder, const float* weightsValues, float bias){
    //NN_ASSIGN_COMPUTATION [Neuron Number 1] [Input Size] [Input Save Order] [weights values] [bias]
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
}