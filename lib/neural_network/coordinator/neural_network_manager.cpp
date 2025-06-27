#include "neural_network_manager.h"
#define NEURAL_NET_IMPL


/***
 * Neural Network Computations Assignment table
 *
 * mTable[TableMaxSize] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the metrics table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the metrics table, including:
 * * * .numberOfItems - The current number of entries in the metrics table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the mTable.
 *
 * childrenTable - A pointer to TTable, used for accessing the children table.
 *
 * valuesPubSub[TableMaxSize] - Preallocated memory for storing the published and subscribed values of each node
 ***/
TableEntry ntnTable[TableMaxSize];
TableInfo NTNTable = {
        .numberOfItems = 0,
        .isEqual = isIDEqual,
        .table = ntnTable,
        .setKey = setNeuronId,
        .setValue = setNeuronMap,
};
TableInfo* neuronToNodeTable  = &NTNTable;

bool isIDEqual(void* av, void* bv) {
    int *a = (int*) av;
    int *b = (int*) bv;
    if(a == b)return true;

    return false;
}

void setNeuronId(void* av, void* bv){
    int *a = (int*) av;
    int *b = (int*) bv;
    a = b;
}

void setID(void* av, void* bv){
    int *a = (int*) av;
    int *b = (int*) bv;
    a = b;
}

void setNeuronMap(void* av, void* bv){
    NeuronMap *a = (NeuronMap*) av;
    NeuronMap *b = (NeuronMap*) bv;

    a->nodeIP[0] = b->nodeIP[0];
    a->nodeIP[1] = b->nodeIP[1];
    a->nodeIP[2] = b->nodeIP[2];
    a->nodeIP[3] = b->nodeIP[3];

    a->layer = b->layer;
    a->indexInLayer = b->indexInLayer;
}


void distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes){
    uint32_t neuronPerNodeCount = 0,*inputIndexMap;
    int assignedDevices = 0, messageOffset = 0;
    uint32_t numHiddenNeurons =0, neuronsPerNode;
    // Initialize the neuron ID to the first neuron in the first hidden layer (i.e., the first ID after the last input neuron)
    uint32_t currentNeuronId= net->layers[0].numInputs;
    char tmpBuffer[150];

    // Count the neurons in the hidden layers, as only these will be assigned to nodes in the network
    for (int i = 0; i < net->numHiddenLayers; i++) {
        numHiddenNeurons += net->layers[i].numOutputs;
    }

    // Calculate how many neurons will be assigned to each node
    neuronsPerNode = ceil( numHiddenNeurons / nrNodes);

    // Encode the message header for the first node’s neuron assignments
    encodeMessageHeader(tmpBuffer, sizeof(tmpBuffer),NN_ASSIGN_COMPUTATION);
    messageOffset += snprintf(largeSendBuffer, sizeof(largeSendBuffer),"%s",tmpBuffer);

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
            LOG(APP, INFO, "Layer nr:%i reserved for root calculation\n",net->numLayers-1);
            continue;
        }

        for (int j = 0; j < net->layers[i].numOutputs; j++) { // For each neuron in each layer

            LOG(APP, INFO, "Node IP: %i.%i.%i.%i\n",nodes[assignedDevices][0],nodes[assignedDevices][1],nodes[assignedDevices][2],nodes[assignedDevices][3]);

            encodeAssignNeuronMessage(tmpBuffer, sizeof(tmpBuffer),
                                           currentNeuronId,net->layers[i].numInputs,inputIndexMap,
                                           &net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j]);

            messageOffset += snprintf(largeSendBuffer + messageOffset, sizeof(largeSendBuffer) - messageOffset,"%s",tmpBuffer);

            // Increment the count of neurons assigned to this node, and the current NeuronID
            currentNeuronId ++;
            neuronPerNodeCount++;

            // When the number of neurons assigned to the current device reaches the expected amount, move on to the next device in the list.
            if(neuronPerNodeCount == neuronsPerNode){
                // Move to the next node, except when all neurons have been assigned i.e., the last neuron of the final layer
                if((j != net->layers[i].numOutputs - 1) || (i != net->numLayers-2)) assignedDevices ++;
                neuronPerNodeCount = 0;

                if(assignedDevices>=nrNodes){
                    LOG(APP, ERROR, "ERROR: The number of assigned devices exceeded the total available nodes for neuron assignment.\n");
                    return;
                }

                //TODO Send the message
                LOG(APP, INFO, "Encoded Message: NN_ASSIGN_COMPUTATION [Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]\n");
                LOG(APP, INFO, "Encoded Message: %s size:%i\n",largeSendBuffer, strlen(largeSendBuffer));

                // Reset the send buffer and message offset to prepare for the next node's neuron assignments
                messageOffset = 0;
                strcpy(largeSendBuffer,"");
                // Encode the message header for the next node’s neuron assignments
                encodeMessageHeader(tmpBuffer, sizeof(tmpBuffer),NN_ASSIGN_COMPUTATION);
                messageOffset += snprintf(largeSendBuffer, sizeof(largeSendBuffer),"%s",tmpBuffer);
            }
        }

        delete [] inputIndexMap;
    }

}


void distributeOutputs(){

}


int encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type){
    if(type == NN_ASSIGN_COMPUTATION){
        return snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_COMPUTATION);
    }
    return 0;
}

int encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize, uint32_t neuronId, uint32_t inputSize, uint32_t * inputSaveOrder, const float* weightsValues, float bias){
    /*** Estimated size of a message assigning a neuron, assuming:
     - Neuron ID and input size each take 2 characters
     - One space between each element
     - Input size of 16
     - Each weight and the bias are represented with 2 decimal places
     Total size breakdown: 144
     2 (Neuron ID) + 1 (space) + 2 (input size) + 1 (space) +
     16*2 (input order indices, 2 chars each) + 16 (spaces) +
     16*4 (weights, ~4 chars each including decimal) + 16 (spaces) +
     4 (bias, ~4 chars)***/

    //|[Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]
    int offset = 0;

    //Encode: NN_ASSIGN_COMPUTATION [Neuron Number 1] [Input Size]
    offset = snprintf(messageBuffer,bufferSize,"|");

    //Encode the neuronID and inputSize
    offset += snprintf(messageBuffer+ offset,bufferSize - offset,"%i %i ",neuronId,inputSize);

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
    return offset;

}

void encodeAssignOutputMessage(char* messageBuffer, size_t bufferSize, int* outputNeuronIds, int nNeurons, uint8_t IPs[][4], uint8_t nNodes){
    int offset = 0;
    //[neuron ID1] [neuron ID2] ... [IP Address 1] [IP Address 2] ...

    // Encode the IDs of neurons whose outputs should be sent to specific IP addresses
    for (int i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",outputNeuronIds[i]);
    }

    // Encode the target IP addresses for the outputs of the specified neuron IDs
    for (int i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d.%d.%d.%d ",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
    }
}

void encodePubSubInfo(char* messageBuffer, size_t bufferSize, int* neuronIds, int nNeurons, int* subTopics, int nSubTopics, int* pubTopics, int nPubTopics ){
    int offset = 0;
    // [neuron ID1] [neuron ID2] ... [Number of Subscriptions] [Subscription 1] [Subscription 2] ... [Number of Publications] [Pub 1] [Pub 2] ...

    // Encode the IDs of neurons that the Pub/sub info is about
    for (int i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",neuronIds[i]);
    }

    // Encode the total number of subscriptions
    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",nSubTopics);

    // Encode the list of subscriptions
    for (int i = 0; i < nSubTopics; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",subTopics[i]);
    }

    // Encode the total number of published topics
    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",nPubTopics);

    // Encode the list of published topics
    for (int i = 0; i < nPubTopics; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",pubTopics[i]);
    }

}