#ifdef NEURAL_NET_IMPL
#define NEURAL_NET_IMPL // Put the #define before the include to have access to the NN parameters
#endif

#include "neural_network_manager.h"


#define TOTAL_NEURONS 12

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
TableEntry ntnTable[TOTAL_NEURONS];
TableInfo NTNTable = {
        .numberOfItems = 0,
        .isEqual = isIDEqual,
        .table = ntnTable,
        .setKey = setNeuronId,
        .setValue = setNeuronEntry,
};
TableInfo* neuronToNodeTable  = &NTNTable;

uint8_t neurons[TOTAL_NEURONS];
NeuronEntry neuronMap[TOTAL_NEURONS];



bool isIDEqual(void* av, void* bv) {
    uint8_t *a = (uint8_t *) av;
    uint8_t *b = (uint8_t *) bv;
    if(*a == *b)return true;

    return false;
}

void setNeuronId(void* av, void* bv){
    uint8_t *a = (uint8_t *) av;
    uint8_t *b = (uint8_t *) bv;
    *a = *b;
}


void setNeuronEntry(void* av, void* bv){
    NeuronEntry *a = (NeuronEntry*) av;
    NeuronEntry *b = (NeuronEntry*) bv;

    a->nodeIP[0] = b->nodeIP[0];
    a->nodeIP[1] = b->nodeIP[1];
    a->nodeIP[2] = b->nodeIP[2];
    a->nodeIP[3] = b->nodeIP[3];

    a->layer = b->layer;
    a->indexInLayer = b->indexInLayer;
}

void initNeuralNetwork(){
    tableInit(neuronToNodeTable,neurons,neuronMap, sizeof(uint8_t),sizeof(NeuronEntry));
}

void printNeuronEntry(TableEntry* Table){
    LOG(APP,INFO,"Neuron %hhu → NodeIP[%hhu.%hhu.%hhu.%hhu] (Layer: %hhu) (Index in Layer: %hhu) \n",
         *(uint8_t*)Table->key,((NeuronEntry *)Table->value)->nodeIP[0],((NeuronEntry *)Table->value)->nodeIP[1]
        ,((NeuronEntry *)Table->value)->nodeIP[2],((NeuronEntry *)Table->value)->nodeIP[3],
        ((NeuronEntry *)Table->value)->layer,((NeuronEntry *)Table->value)->indexInLayer);
}

void printNeuronTableHeader(){
    LOG(APP,INFO,"((((((((((((((((((((((( Neuron To Node Table )))))))))))))))))))))))))))\n");

}
void distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes){
    uint8_t neuronPerNodeCount = 0,*inputIndexMap;
    int assignedDevices = 0, messageOffset = 0;
    uint8_t numHiddenNeurons =0, neuronsPerNode;
    // Initialize the neuron ID to the first neuron in the first hidden layer (i.e., the first ID after the last input neuron)
    uint8_t currentNeuronId= net->layers[0].numInputs;
    char tmpBuffer[150];
    NeuronEntry neuronEntry;

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

    for (uint8_t i = 0; i < net->numLayers; i++) { //For each hidden layer

        /***Initialize the input index mapping before processing each layer. The inputIndexMapping specifies the order
         in which the node should store input values. It corresponds to an ordered list of neuron IDs from the previous
         layer, since those neurons serve as inputs to the current layer.***/
        inputIndexMap = new uint8_t [net->layers[i].numInputs];
        for (uint8_t j = 0; j < net->layers[i].numInputs ; j++){
            // For the first hidden layer, the input index mapping corresponds to the neuron IDs of the input layer (ranging from 0 to nrInputs - 1).
            //if(i == 0)inputIndexMap[j] = j;
            // For subsequent hidden layers range from (currentNeuronId - (neuronsInPreviousLayer)) to (currentNeuronId)
            inputIndexMap[j] = currentNeuronId+(j-net->layers[i].numInputs);
        }


        for (uint8_t j = 0; j < net->layers[i].numOutputs; j++) { // For each neuron in each layer

            // The root node (the node running this algorithm) is responsible for computing the output layer.
            if(i == net->numLayers - 1){
                //TODO this node computed the output layer
                LOG(APP,DEBUG,"neuronToNodeTable Size: %i\n",neuronToNodeTable->numberOfItems);
                LOG(APP,DEBUG,"neuron id:%hhu MyIP:%hhu.%hhu.%hhu.%hhu Layer:%hhu Index in Layer:%hhu\n",currentNeuronId,myIP[0],myIP[1],myIP[2],myIP[3],i,j);
                assignIP(neuronEntry.nodeIP,myIP);
                neuronEntry.layer = i;
                neuronEntry.indexInLayer = j;
                tableAdd(neuronToNodeTable,&currentNeuronId,&neuronEntry);
                // Increment the count of neurons assigned to this node, and the current NeuronID
                currentNeuronId ++;
                neuronPerNodeCount++;/******/
                continue;
            }

            encodeAssignNeuronMessage(tmpBuffer, sizeof(tmpBuffer),
                                           currentNeuronId,net->layers[i].numInputs,inputIndexMap,
                                           &net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j]);

            messageOffset += snprintf(largeSendBuffer + messageOffset, sizeof(largeSendBuffer) - messageOffset,"%s",tmpBuffer);

            // Add the neuron-to-node mapping to the table
            assignIP(neuronEntry.nodeIP,nodes[assignedDevices]);
            neuronEntry.layer = i;
            neuronEntry.indexInLayer = j;
            tableAdd(neuronToNodeTable,&currentNeuronId,&neuronEntry);

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
                //LOG(APP, INFO, "Encoded Message: NN_ASSIGN_COMPUTATION [Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]\n");
                //LOG(APP, INFO, "Encoded Message: %s size:%i\n",largeSendBuffer, strlen(largeSendBuffer));

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


void distributeOutputsV2(uint8_t nodes[][4],uint8_t nrNodes){
    uint8_t currentIP[4]={0,0,0,0}, lastIP[4] ={0,0,0,0},*neuronId, *neuronId2;
    NeuronEntry *neuronEntry,*neuronEntry2;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    uint8_t inputNodesIPs[TableMaxSize][4], nNodes = 0;
    uint8_t lastLayer=0,currentLayer=0;


    for (int i = 0; i < neuronToNodeTable->numberOfItems; i++) {
        neuronId = (uint8_t*)tableKey(neuronToNodeTable,i);
        neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

        if (neuronEntry != nullptr){ //Safe Guard against nullptr

            assignIP(currentIP,neuronEntry->nodeIP);
            currentLayer = neuronEntry->layer;

            //If the node that computes the current neuron is equal to the last node that computed the last neuron and the
            //current node is situated in the same layer as the last Neuron then those outputs can be aggregated because are going
            //to be set to neurons of the next layer
            if(isIPEqual(currentIP,lastIP) && currentLayer == lastLayer){
                outputNeurons[nNeurons] = *neuronId;
                nNeurons++;
            }else{

                for (int j = 0; j < neuronToNodeTable->numberOfItems; j++){
                    neuronId2 = (uint8_t*)tableKey(neuronToNodeTable,j);
                    neuronEntry2 = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

                    if (neuronEntry2 != nullptr){
                        if(neuronEntry2->layer == lastLayer + 1){
                            assignIP(inputNodesIPs[nNodes],neuronEntry2->nodeIP);
                            nNodes++;
                        }
                    }
                }

                //Todo encode the message

                encodeAssignOutputMessage(largeSendBuffer,sizeof(largeSendBuffer),outputNeurons,nNeurons,inputNodesIPs,nNodes);
            }
        }
    }

}


void distributeOutputs(uint8_t nodes[][4],uint8_t nrNodes){
    uint8_t *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    uint8_t inputNodesIPs[TableMaxSize][4], nNodes = 0;


    /***  The messages in this function are sent layer by layer.  At each node, messages are made based on the aggregation of
     * neurons computed in the same layer, since these neurons need to send their outputs to the same neurons or nodes
     * in the next layer. ***/
    for (uint8_t i = 0; i < network.numLayers; i++) {

        LOG(APP,INFO,"Layer: %hhu\n",i);

        /*** The neurons in the next layer require the outputs of the current layer as input. Therefore, we need to
         * identify which nodes are responsible for computing the next layer's neurons. These are the nodes to which
         * the current layer's neurons must send their outputs. ***/
        for (int l = 0; l < neuronToNodeTable->numberOfItems; l++) {
            neuronId = (uint8_t*)tableKey(neuronToNodeTable,l);
            neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);
            if(neuronEntry != nullptr){
                // Check if the current node is responsible for computing a neuron in the next layer and is not already in the list
                if(!isIPinList(neuronEntry->nodeIP,inputNodesIPs,nNodes) && (neuronEntry->layer == i+1)){
                    assignIP(inputNodesIPs[nNodes],neuronEntry->nodeIP);
                    LOG(APP,INFO,"IP of the next layer node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry->nodeIP[0],neuronEntry->nodeIP[1],neuronEntry->nodeIP[2],neuronEntry->nodeIP[3]);
                    nNodes++;
                }
            }
        }

        /*** For each node in the current layer, check which neurons it computes and send a message containing the
         * IP addresses of the neurons that require its output.***/
        for (uint8_t j = 0; j < nrNodes; j++) {
            LOG(APP,INFO,"Current IP: %hhu.%hhu.%hhu.%hhu computes:\n",nodes[j][0],nodes[j][1],nodes[j][2],nodes[j][3]);

            // Search the neuronToNodeTable for neurons at a specific layer that are computed by a specific node
            for (int k = 0; k < neuronToNodeTable->numberOfItems; ++k) {
                neuronId = (uint8_t*)tableKey(neuronToNodeTable,k);
                neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

                if(neuronEntry != nullptr){
                    if(isIPEqual(neuronEntry->nodeIP,nodes[j]) && neuronEntry->layer == i){
                        LOG(APP,INFO,"Neuron ID: %hhu \n",*neuronId);
                        outputNeurons[nNeurons] = *neuronId;
                        nNeurons ++;
                    }
                }
            }
            LOG(APP,DEBUG,"number of neurons: %hhu number of nodes: %hhu\n",nNeurons,nNodes);
            encodeAssignOutputMessage(largeSendBuffer,sizeof(largeSendBuffer),outputNeurons,nNeurons,inputNodesIPs,nNodes);
            LOG(APP,INFO,"Encoded message: %s\n",largeSendBuffer);
            //Todo send Message for the node

            nNeurons = 0;
        }

        nNodes = 0;
    }

}

void distributeOutputsByNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]){
    uint8_t *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    uint8_t inputNodesIPs[TableMaxSize][4], nNodes = 0;
    int offset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);

    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_OUTPUTS);
    offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

    /***  The messages in this function are constructed layer by layer. Messages are made based on aggregation of the
     * neurons computed in the same layer, since these neurons need to send their outputs to the same neurons or nodes
     * in the next layer. ***/
    for (uint8_t i = 0; i < network.numLayers; i++) {
        /*** The neurons in the next layer require the outputs of the current layer as input. Therefore, we need to
         * identify which nodes are responsible for computing the next layer's neurons. These are the nodes to which
         * the current layer's neurons must send their outputs. ***/
        for (int l = 0; l < neuronToNodeTable->numberOfItems; l++) {
            neuronId = (uint8_t*)tableKey(neuronToNodeTable,l);
            neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);
            if(neuronEntry != nullptr){
                // Check if the current node is responsible for computing a neuron in the next layer and is not already in the list
                if(!isIPinList(neuronEntry->nodeIP,inputNodesIPs,nNodes) && (neuronEntry->layer == i+1)){
                    assignIP(inputNodesIPs[nNodes],neuronEntry->nodeIP);
                    //LOG(APP,INFO,"IP of the next layer node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry->nodeIP[0],neuronEntry->nodeIP[1],neuronEntry->nodeIP[2],neuronEntry->nodeIP[3]);
                    nNodes++;
                }
            }
        }

        //LOG(APP,INFO,"Current IP: %hhu.%hhu.%hhu.%hhu computes:\n",targetNodeIP[0],targetNodeIP[1],targetNodeIP[2],targetNodeIP[3]);

        /*** Identify the neurons computed by the target node and build a message
           * with the IP addresses of the neurons that depend on its output.  ***/
        for (int k = 0; k < neuronToNodeTable->numberOfItems; ++k) {
            neuronId = (uint8_t*)tableKey(neuronToNodeTable,k);
            neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

            if(neuronEntry != nullptr){
                //Search in the neuronToNodeTable for the neurons at a specific layer computed by a specif node
                if(isIPEqual(neuronEntry->nodeIP,targetNodeIP) && neuronEntry->layer == i){
                    //LOG(APP,INFO,"Neuron ID: %hhu \n",*neuronId);
                    outputNeurons[nNeurons] = *neuronId;
                    nNeurons ++;
                }
            }
        }

        // If the node computes any neurons in this layer, include its information in the message along with the IPs to which it should forward the computation
        if(nNeurons != 0){
            //LOG(APP,DEBUG,"number of neurons: %hhu number of targetNodeIP: %hhu\n",nNeurons,nNodes);
            encodeAssignOutputMessage(tmpBuffer,tmpBufferSize,outputNeurons,nNeurons,inputNodesIPs,nNodes);
            offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);
            //Todo send Message for the node
        }

        nNeurons = 0;
        nNodes = 0;
    }

}


void encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type){
    if(type == NN_ASSIGN_COMPUTATION){
        snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_COMPUTATION);
    }else if(type == NN_ASSIGN_OUTPUTS){
        snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_OUTPUTS);
    }
}

int encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize, uint8_t neuronId, uint8_t inputSize, uint8_t * inputSaveOrder, const float* weightsValues, float bias){
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
    offset += snprintf(messageBuffer+ offset,bufferSize - offset,"%hhu %hhu ",neuronId,inputSize);

    //Encode the [Input Save Order] vector
    for (uint8_t i = 0; i < inputSize; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%hhu ",inputSaveOrder[i]);
    }

    //Encode the [weights values] vector
    for (uint8_t i = 0; i < inputSize; i++) {
        //%g format specifier automatically chooses the most compact representation, removing unnecessary trailing zeros
        //2.0 -> 2 , 2.1->2.1
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%g ",weightsValues[i]);
    }

    offset += snprintf(messageBuffer + offset, bufferSize - offset,"%g",bias);
    return offset;

}

void encodeAssignOutputMessage(char* messageBuffer, size_t bufferSize, uint8_t * outputNeuronIds, uint8_t nNeurons, uint8_t IPs[][4], uint8_t nNodes){
    int offset = 0;
    //|[neuron ID1] [neuron ID2] ... [IP Address 1] [IP Address 2] ...

    offset = snprintf(messageBuffer, bufferSize, "|");

    // Encode the IDs of neurons whose outputs should be sent to specific IP addresses
    for (uint8_t i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",outputNeuronIds[i]);
    }

    // Encode the target IP addresses for the outputs of the specified neuron IDs
    for (uint8_t i = 0; i < nNodes; i++) {
        if(i != nNodes -1)offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu ",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
        else offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
    }
}

void encodePubSubInfo(char* messageBuffer, size_t bufferSize, uint8_t * neuronIds, uint8_t nNeurons, uint8_t * subTopics, uint8_t nSubTopics, uint8_t * pubTopics, uint8_t nPubTopics ){
    int offset = 0;
    // [neuron ID1] [neuron ID2] ... [Number of Subscriptions] [Subscription 1] [Subscription 2] ... [Number of Publications] [Pub 1] [Pub 2] ...

    // Encode the IDs of neurons that the Pub/sub info is about
    for (uint8_t i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",neuronIds[i]);
    }

    // Encode the total number of subscriptions
    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",nSubTopics);

    // Encode the list of subscriptions
    for (uint8_t i = 0; i < nSubTopics; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",subTopics[i]);
    }

    // Encode the total number of published topics
    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",nPubTopics);

    // Encode the list of published topics
    for (uint8_t i = 0; i < nPubTopics; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",pubTopics[i]);
    }

}


bool isIPinList(uint8_t *searchIP,uint8_t list[][4],uint8_t nElements){
    for (uint8_t i = 0; i < nElements; i++) {
        if(isIPEqual(list[i],searchIP)){
            return true;
        }
    }
    return false;
}