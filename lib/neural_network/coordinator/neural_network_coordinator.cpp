//#ifndef NEURAL_NET_IMPL
//#define NEURAL_NET_IMPL // Put the #define before the include to have access to the NN parameters
//#endif

#include "neural_network_coordinator.h"


#define NODES_PER_ESP8266 1
#define NODES_PER_ESP32 3
#define NODES_PER_RPI 10


/***uint8_t numESP8266Workers=0;
uint8_t numESP32Workers=0;
uint8_t numRPiWorkers=0;***/

/***
 * Neural Network Computations Assignment table
 *
 * mTable[TABLE_MAX_SIZE] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the metrics table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the metrics table, including:
 * * * .numberOfItems - The current number of entries in the metrics table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the mTable.
 *
 * childrenTable - A pointer to TTable, used for accessing the children table.
 *
 * valuesPubSub[TABLE_MAX_SIZE] - Preallocated memory for storing the published and subscribed values of each node
 ***/
TableEntry ntnTable[TOTAL_NEURONS];
TableInfo NTNTable = {
        .numberOfItems = 0,
        .maxNumberOfItems = TOTAL_NEURONS,
        .isEqual = isIDEqual,
        .table = ntnTable,
        .setKey = setNeuronId,
        .setValue = setNeuronEntry,
};
TableInfo* neuronToNodeTable  = &NTNTable;

NeuronId neurons[TOTAL_NEURONS];
NeuronEntry neuronMap[TOTAL_NEURONS];



bool isIDEqual(void* av, void* bv) {
    NeuronId *a = (NeuronId *) av;
    NeuronId *b = (NeuronId *) bv;
    if(*a == *b)return true;

    return false;
}

void setNeuronId(void* av, void* bv){
    NeuronId *a = (NeuronId *) av;
    NeuronId *b = (NeuronId *) bv;
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
    a->isAcknowledged = b->isAcknowledged;
}

NeuralNetworkCoordinator::NeuralNetworkCoordinator() {
    initNeuralNetwork();
}

void NeuralNetworkCoordinator::initNeuralNetwork(){
    tableInit(neuronToNodeTable,neurons,neuronMap, sizeof(NeuronId),sizeof(NeuronEntry));
}

void printNeuronEntry(TableEntry* Table){
    LOG(APP,INFO,"Neuron ID %hhu → NodeIP[%hhu.%hhu.%hhu.%hhu] (Layer: %hhu) (Index in Layer: %hhu) (isAcknowledged: %d) \n",
         *(NeuronId*)Table->key,((NeuronEntry *)Table->value)->nodeIP[0],((NeuronEntry *)Table->value)->nodeIP[1]
        ,((NeuronEntry *)Table->value)->nodeIP[2],((NeuronEntry *)Table->value)->nodeIP[3],
        ((NeuronEntry *)Table->value)->layer,((NeuronEntry *)Table->value)->indexInLayer,
        ((NeuronEntry *)Table->value)->isAcknowledged);
}

void printNeuronTableHeader(){
    LOG(APP,INFO,"((((((((((((((((((((((( Neuron To Node Table )))))))))))))))))))))))))))\n");

}

void NeuralNetworkCoordinator::distributeNeuralNetwork(const NeuralNetwork *net, uint8_t nodes[][4],uint8_t nrNodes){
    uint8_t neuronPerNodeCount = 0,*inputIndexMap;
    uint8_t numHiddenNeurons =0, neuronsPerNode;
    uint8_t myIP[4]={0,0,0,0};
    int assignedDevices = 0, messageOffset = 0;
    // Initialize the neuron ID to the first neuron in the first hidden layer (i.e., the first ID after the last input neuron)
    uint8_t currentNeuronId= net->layers[0].numInputs;
    char tmpBuffer[150];
    size_t tmpBufferSize= sizeof(tmpBuffer);
    NeuronEntry neuronEntry;

    // Count the neurons in the hidden layers, as only these will be assigned to nodes in the network
    for (int i = 0; i < net->numHiddenLayers; i++) {
        numHiddenNeurons += net->layers[i].numOutputs;
    }

    // Calculate how many neurons will be assigned to each node
    neuronsPerNode = ceil( numHiddenNeurons / nrNodes);

    // Encode the message header for the first node’s neuron assignments
    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
    messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);

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


        for (uint8_t j = 0; j < net->layers[i].numOutputs; j++){ // For each neuron in each layer

            // The root node (the node running this algorithm) is responsible for computing the output layer.
            if(i == net->numLayers - 1){
                // Add the neuron-to-node mapping to the table
                network.getNodeIP(myIP);
                assignIP(neuronEntry.nodeIP,myIP);
                neuronEntry.layer = i+1;
                neuronEntry.indexInLayer = j;
                neuronEntry.isAcknowledged = true;
                tableAdd(neuronToNodeTable,&currentNeuronId,&neuronEntry);

                // Stores the parameters assigned to this node for later use in computing the output neuron values.
                neuronCore.configureNeuron(currentNeuronId,net->layers[i].numInputs,&net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j], inputIndexMap);

                // Increment the count of neurons assigned to this node, and the current NeuronID
                currentNeuronId ++;
                continue;
            }

            encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                           currentNeuronId,net->layers[i].numInputs,inputIndexMap,
                                           &net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j]);

            messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset,"%s",tmpBuffer);

            // Add the neuron-to-node mapping to the table
            assignIP(neuronEntry.nodeIP,nodes[assignedDevices]);
            neuronEntry.layer = i+1;
            neuronEntry.indexInLayer = j;
            neuronEntry.isAcknowledged = false;

            tableAdd(neuronToNodeTable,&currentNeuronId,&neuronEntry);

            // Increment the count of neurons assigned to this node, and the current NeuronID
            currentNeuronId ++;
            neuronPerNodeCount++;

            // When the number of neurons assigned to the current device reaches the expected amount, move on to the next device in the list.
            if(neuronPerNodeCount == neuronsPerNode){

                //Send the message to the node assigning him the neurons
                //LOG(APP, INFO, "Encoded Message: NN_ASSIGN_COMPUTATION [Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]\n");
                //LOG(APP, INFO, "Encoded Message: %s size:%i\n",largeSendBuffer, strlen(largeSendBuffer));
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodes[assignedDevices]);

                // Move to the next node, except when all neurons have been assigned i.e., the last neuron of the final layer
                if((j != net->layers[i].numOutputs - 1) || (i != net->numLayers-2)) assignedDevices ++;
                neuronPerNodeCount = 0;

                if(assignedDevices>=nrNodes){
                    LOG(APP, ERROR, "ERROR: The number of assigned devices exceeded the total available nodes for neuron assignment.\n");
                    return;
                }

                // Reset the payload buffer and message offset to prepare for the next node's neuron assignments
                messageOffset = 0;
                strcpy(appPayload,"");
                // Encode the message header for the next node’s neuron assignments
                encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
                messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);
            }
        }

        delete [] inputIndexMap;
    }

    neuronAssignmentTime = getCurrentTime();
    areNeuronsAssigned = true;
}



void NeuralNetworkCoordinator::distributeNeuralNetworkBalanced(const NeuralNetwork *net, uint8_t devices[][4],uint8_t nrDevices, uint8_t neuronsPerDevice[]){
    uint8_t neuronPerNodeCount = 0,*inputIndexMap;
    int assignedDevices = 0, messageOffset = 0;
    // Initialize the neuron ID to the first neuron in the first hidden layer (i.e., the first ID after the last input neuron)
    uint8_t currentNeuronId= net->layers[0].numInputs;
    char tmpBuffer[150];
    size_t tmpBufferSize= sizeof(tmpBuffer);
    NeuronEntry neuronEntry;

    // Encode the message header for the first node’s neuron assignments
    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
    messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);

    //LOG(APP, INFO, "Neural network initialized with %d neurons (avg. %d neurons per node)\n", net->numNeurons, neuronsPerNode);

    for (uint8_t i = 0; i < net->numLayers-1; i++) { //For each hidden layer

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

        for (uint8_t j = 0; j < net->layers[i].numOutputs; j++){ // For each neuron in each layer

            encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                      currentNeuronId,net->layers[i].numInputs,inputIndexMap,
                                      &net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j]);

            messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset,"%s",tmpBuffer);

            // Add the neuron-to-node mapping to the table
            assignIP(neuronEntry.nodeIP,devices[assignedDevices]);
            neuronEntry.layer = i+1;
            neuronEntry.indexInLayer = j;
            neuronEntry.isAcknowledged = false;

            tableAdd(neuronToNodeTable,&currentNeuronId,&neuronEntry);

            // Increment the count of neurons assigned to this node, and the current NeuronID
            currentNeuronId ++;
            neuronPerNodeCount++;

            /***
             * When the number of neurons assigned to the current device reaches the expected amount, send the assignment
             * message and move on to the next device in the list. If the current device has fewer than the expected number
             * of neurons and no more neurons remain to be assigned, send the message to the node with the neurons already assigned.
             ***/
            if(neuronPerNodeCount == neuronsPerDevice[assignedDevices] || (i==net->numLayers-2 && j==net->layers[i].numOutputs-1)){
                //Send the message to the node assigning him the neurons
                //LOG(APP, INFO, "Encoded Message: NN_ASSIGN_COMPUTATION [Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]\n");
                //LOG(APP, INFO, "Encoded Message: %s size:%i\n",largeSendBuffer, strlen(largeSendBuffer));
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,devices[assignedDevices]);

                // Move to the next node, except when all neurons have been assigned i.e., the last neuron of the final layer
                if((j != net->layers[i].numOutputs - 1) || (i != net->numLayers-2)) assignedDevices ++;
                neuronPerNodeCount = 0;

                if(assignedDevices>=nrDevices){
                    LOG(APP, ERROR, "ERROR: The number of assigned devices exceeded the total available devices for neuron assignment.\n");
                    return;
                }

                // Reset the payload buffer and message offset to prepare for the next node's neuron assignments
                messageOffset = 0;
                strcpy(appPayload,"");
                // Encode the message header for the next node’s neuron assignments
                encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
                messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);
            }
        }

        delete [] inputIndexMap;
    }

    neuronAssignmentTime = getCurrentTime();
    areNeuronsAssigned = true;
}


void NeuralNetworkCoordinator::assignOutputTargetsToNetwork(uint8_t nodes[][4],uint8_t nrNodes){
    uint8_t *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    uint8_t inputNodesIPs[TABLE_MAX_SIZE][4], nNodes = 0;


    /***  The messages in this function are sent layer by layer.  At each node, messages are made based on the aggregation of
     * neurons computed in the same layer, since these neurons need to send their outputs to the same neurons or nodes
     * in the next layer. ***/
    for (uint8_t i = 0; i < neuralNetwork.numLayers; i++) {

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
            encodeAssignOutputMessage(appPayload,sizeof(appPayload),outputNeurons,nNeurons,inputNodesIPs,nNodes);

            //LOG(APP,INFO,"Encoded message: %s\n",appPayload);
            //Todo send Message for the node
            network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodes[j]);
            nNeurons = 0;
        }

        nNodes = 0;
    }

}

void NeuralNetworkCoordinator::assignOutputTargetsToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]){
    NeuronId *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    uint8_t inputNodesIPs[TABLE_MAX_SIZE][4], nNodes = 0;
    int offset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    bool neuronsAssignedToNode=false;

    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_OUTPUT_TARGETS);
    offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

    /***  The messages in this function are constructed layer by layer. Messages are made based on aggregation of the
     * neurons computed in the same layer, since these neurons need to send their outputs to the same neurons or nodes
     * in the next layer. ***/
    for (uint8_t i = 0; i < neuralNetwork.numLayers+1; i++){
        /*** The neurons in the next layer require the outputs of the current layer as input. Therefore, we need to
         * identify which nodes are responsible for computing the next layer's neurons. These are the nodes to which
         * the current layer's neurons must send their outputs. ***/
        for (int l = 0; l < neuronToNodeTable->numberOfItems; l++){
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
                //Search in the neuronToNodeTable for the neurons at a specific layer computed by the target node that have not been acknowledged
                if(isIPEqual(neuronEntry->nodeIP,targetNodeIP) && neuronEntry->layer == i && !neuronEntry->isAcknowledged){
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
            neuronsAssignedToNode = true;
        }

        nNeurons = 0;
        nNodes = 0;
    }

    // Check if the target node has any assigned neurons. This safeguards against cases where the function is given a
    // targetIP with no neuron assignments, in such cases, there's no need to send a message.
    if(neuronsAssignedToNode){
        network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,targetNodeIP);
    }


}

void NeuralNetworkCoordinator::assignOutputTargetsToNeurons(char* messageBuffer,size_t bufferSize,NeuronId *neuronIDs,uint8_t nNeurons,uint8_t targetNodeIP[4]){
    NeuronId *neuronId,*neuronId2;
    NeuronEntry *neuronEntry,*neuronEntry2;
    uint8_t inputNodesIPs[TABLE_MAX_SIZE][4], nNodes = 0;
    int offset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);

    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_OUTPUT_TARGETS);
    offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

    /***  This function builds the message neuron by neuron. It finds each neuron to assign outputs to,
     * retrieves its output targets and appends the neuron and corresponding targets to the message.***/
    for (int i = 0; i < neuronToNodeTable->numberOfItems; i++){
        neuronId = (uint8_t*)tableKey(neuronToNodeTable,i);
        neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

        if (neuronEntry == nullptr)continue;
        //LOG(APP,DEBUG,"Neuron: %hhu\n",*neuronId);

        // If the current neuronId is not in the list of neurons that should receive the output target assignment skip it
        if (!isNeuronInList(neuronIDs,nNeurons,*neuronId)) continue;

        /*** The neurons in the next layer require the outputs of the current layer as input. Therefore, we need to
         * identify which nodes are responsible for computing the next layer's neurons. These are the nodes to which
         * the current layer's neurons must send their outputs. ***/
        for (int l = 0; l < neuronToNodeTable->numberOfItems; l++){
            neuronId2 = (uint8_t*)tableKey(neuronToNodeTable,l);
            neuronEntry2 = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId2);
            if(neuronEntry2 != nullptr){
                // Check if the current node is responsible for computing a neuron in the next layer and is not already in the list
                if(!isIPinList(neuronEntry2->nodeIP,inputNodesIPs,nNodes) && (neuronEntry2->layer == neuronEntry->layer+1)){
                    assignIP(inputNodesIPs[nNodes],neuronEntry2->nodeIP);
                    //LOG(APP,INFO,"IP of the next layer node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry->nodeIP[0],neuronEntry->nodeIP[1],neuronEntry->nodeIP[2],neuronEntry->nodeIP[3]);
                    nNodes++;
                }
            }
        }

        encodeAssignOutputMessage(tmpBuffer,tmpBufferSize,neuronId,1,inputNodesIPs,nNodes);
        offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

        nNodes = 0;
    }

    network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,targetNodeIP);

}

void NeuralNetworkCoordinator::distributeInputNeurons(uint8_t inputNodes[][4],uint8_t nrNodes){
    NeuronEntry neuronEntry;
    uint8_t numInputNeurons = neuralNetwork.layers[0].numInputs;
    if(numInputNeurons > nrNodes){
            LOG(APP, ERROR, "Error: number of input neurons exceeds the number of available nodes");
        return;
    }
    for (int i = 0; i < numInputNeurons; i++){
        //Encode ad send the message assigning the input neuron to the physical device
        encodeInputAssignMessage(appPayload, sizeof(appPayload),i);
        network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,inputNodes[i]);

        // Add the neuron-to-node mapping to the table
        assignIP(neuronEntry.nodeIP,inputNodes[i]);
        neuronEntry.layer = 0;
        neuronEntry.indexInLayer = i;
        neuronEntry.isAcknowledged = false;

        tableAdd(neuronToNodeTable,&i,&neuronEntry);

    }
}

void NeuralNetworkCoordinator::distributeOutputNeurons(const NeuralNetwork *net,uint8_t outputDevice[4]){
    uint8_t outputLayer= net->numLayers - 1,*inputIndexMap;
    uint8_t myIP[4];
    network.getNodeIP(myIP);
    NeuronId currentOutputNeuron=0;
    NeuronEntry neuronEntry;
    char tmpBuffer[150];
    size_t tmpBufferSize= sizeof(tmpBuffer);
    int messageOffset=0,neuronStorageIndex=-1;

    // Calculate the first output neuron ID by iterating through the layers and their respective number of inputs
    for (int i = 0; i < net->numLayers; ++i) {
        currentOutputNeuron += net->layers[i].numInputs;
    }

    //LOG(APP,DEBUG,"CurrentOutputNeuronId: %hhu\n",currentOutputNeuron);

    /***Initialize the input index mapping before processing the output layer. The inputIndexMapping specifies the order
     in which the node should store input values. It corresponds to an ordered list of neuron IDs from the previous
     layer, since those neurons serve as inputs to the current layer.***/
    inputIndexMap = new uint8_t [net->layers[outputLayer].numInputs];
    for (uint8_t j = 0; j < net->layers[outputLayer].numInputs ; j++){
        // For the first hidden layer, the input index mapping corresponds to the neuron IDs of the input layer (ranging from 0 to nrInputs - 1).
        //if(i == 0)inputIndexMap[j] = j;
        // For subsequent hidden layers range from (currentNeuronId - (neuronsInPreviousLayer)) to (currentNeuronId)
        inputIndexMap[j] = currentOutputNeuron+(j-net->layers[outputLayer].numInputs);
    }

    if(!isIPEqual(outputDevice,myIP)){
        encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_OUTPUT);
        messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);
    }

    for (uint8_t j = 0; j < net->layers[outputLayer].numOutputs; j++){ // For each neuron in output layer

        // If the node corresponds to this device, add it to the neuron-to-node table automatically acknowledged.
        if (isIPEqual(outputDevice,myIP)){
            // Add the neuron-to-node mapping to the table
            network.getNodeIP(myIP);
            assignIP(neuronEntry.nodeIP,myIP);
            neuronEntry.layer = outputLayer+1;
            neuronEntry.indexInLayer = j;
            neuronEntry.isAcknowledged = true;

            tableAdd(neuronToNodeTable,&currentOutputNeuron,&neuronEntry);

            // Stores the parameters assigned to this node for later use in computing the output neuron values.
            neuronCore.configureNeuron(currentOutputNeuron,net->layers[outputLayer].numInputs,
                            &net->layers[outputLayer].weights[j * net->layers[outputLayer].numInputs],
                            net->layers[outputLayer].biases[j], inputIndexMap);

            //Get where the current neuron was saved in neuron core
            neuronStorageIndex = neuronCore.getNeuronStorageIndex(currentOutputNeuron);
            // Record the topic published by this neuron in the neuron-to-topic map
            if(neuronStorageIndex != -1) neuronToTopicMap[neuronStorageIndex]= static_cast<int8_t>(outputLayer+1);
        }else{
            // If this node doesn't compute the output layer, we must encode a message
            //assigning the output neurons and their parameters to the correct node.
            encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                      currentOutputNeuron,net->layers[outputLayer].numInputs,inputIndexMap,
                                      &net->layers[outputLayer].weights[j * net->layers[outputLayer].numInputs],net->layers[outputLayer].biases[j]);

            messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset,"%s",tmpBuffer);

            // Add the neuron-to-node mapping to the table
            assignIP(neuronEntry.nodeIP,outputDevice);
            neuronEntry.layer = outputLayer+1;
            neuronEntry.indexInLayer = j;
            neuronEntry.isAcknowledged = false;

            tableAdd(neuronToNodeTable,&currentOutputNeuron,&neuronEntry);
        }

        // Increment the current NeuronID
        currentOutputNeuron ++;

    }

    // If another node is responsible for the output layer, notify it by assigning the output neurons
    if(!isIPEqual(outputDevice,myIP)){
        network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,outputDevice);
    }else{
        // Directly initialize the middleware Pub/Sub table with this device’s information,
        // since the device does not receive messages from itself to update its table.
        int8_t subTopic[1]={(int8_t)outputLayer},pubTopic[1]={(int8_t)(outputLayer+1)};
        network.subscribeAndPublishTopics(subTopic,1,pubTopic,1);

    }

    delete [] inputIndexMap;
}

void NeuralNetworkCoordinator::assignPubSubInfoToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]){
    uint8_t *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    int offset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    bool neuronsAssignedToNode=false;

    //Encode the message header
    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_OUTPUT_TARGETS);
    offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

    /***  The messages in this function are constructed layer by layer. Messages are made based on aggregation of the
     * neurons computed in the same layer, since these neurons need to send their outputs to the same neurons or nodes
     * in the next layer. ***/
    for (int8_t i = 0; i < neuralNetwork.numLayers + 1; i++) {
        /*** Identify the neurons computed by the target node at the current layer and build a message
           * with the subscriptions and publications that the node does ***/
        for (int k = 0; k < neuronToNodeTable->numberOfItems; ++k) {
            neuronId = (uint8_t*)tableKey(neuronToNodeTable,k);
            neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

            if(neuronEntry != nullptr){
                //Search in the neuronToNodeTable for the neurons at a specific layer computed by the specified target node.
                //Only include unacknowledged neurons (because only those need pub/sub info assignment)
                if(isIPEqual(neuronEntry->nodeIP,targetNodeIP) && neuronEntry->layer == i && !neuronEntry->isAcknowledged){
                    //LOG(APP,INFO,"Neuron ID: %hhu \n",*neuronId);
                    outputNeurons[nNeurons] = *neuronId;
                    nNeurons ++;
                }
            }
        }

        // If the node computes any neurons in this layer, include its information in the message along with the IPs to which it should forward the computation
        if(nNeurons != 0){
            //LOG(APP,DEBUG,"number of neurons: %hhu number of targetNodeIP: %hhu\n",nNeurons,nNodes);
            /*** Neurons in a given layer will publish a topic corresponding to their layer number,
                and subscribe to the topic published by neurons in the previous layer.***/
            encodePubSubInfo(tmpBuffer,tmpBufferSize,outputNeurons,nNeurons,(int8_t)(i-1),i);
            //Increment the encoded message with the new pub/sub info
            offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);
            neuronsAssignedToNode=true;
        }

        nNeurons = 0;
    }

    // Check if the target node has any assigned neurons. This safeguards against cases where the function is given a
    // targetIP with no neuron assignments, in such cases, there's no need to send a message.
    if(neuronsAssignedToNode){
        network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,targetNodeIP);
    }


}


void NeuralNetworkCoordinator::assignPubSubInfoToNeuron(char* messageBuffer,size_t bufferSize,NeuronId neuronId){
    NeuronEntry *neuronEntry;
    int offset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    int8_t layerIndex;

    encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_OUTPUT_TARGETS);
    offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

    neuronEntry = (NeuronEntry*)tableRead(neuronToNodeTable,&neuronId);
    if(neuronEntry != nullptr){
        /*** Neurons in a given layer will publish to a topic corresponding to their layer number,
                and subscribe to the topics published by neurons in the previous layer.***/
        if(neuronEntry->layer>127){
            LOG(APP, ERROR, "Error: Neuron layer value (%hhu) exceeds the maximum allowed for int8_t\n", neuronEntry->layer);
            return;
        }
        layerIndex = static_cast<int8_t>(neuronEntry->layer);
        encodePubSubInfo(tmpBuffer,tmpBufferSize,&neuronId,1,static_cast<int8_t>(layerIndex - 1),layerIndex);
        offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);
        network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,neuronEntry->nodeIP);
    }
}

void NeuralNetworkCoordinator::encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type){
    if(type == NN_ASSIGN_COMPUTATION){
        snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_COMPUTATION);
    }else if(type == NN_ASSIGN_OUTPUT){
        snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_OUTPUT);
    }else if(type == NN_ASSIGN_OUTPUT_TARGETS){
        snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_OUTPUT_TARGETS);
    }
}

int NeuralNetworkCoordinator::encodeAssignNeuronMessage(char* messageBuffer, size_t bufferSize, uint8_t neuronId, uint8_t inputSize, uint8_t * inputSaveOrder, const float* weightsValues, float bias){
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

    //Encode the neuronId and inputSize
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

void NeuralNetworkCoordinator::encodeAssignOutputMessage(char* messageBuffer, size_t bufferSize, uint8_t * outputNeuronIds, uint8_t nNeurons, uint8_t IPs[][4], uint8_t nNodes){
    int offset = 0;
    //|[N Neurons] [neuron ID1] [neuron ID2] ...[N Nodes] [IP Address 1] [IP Address 2] ...

    offset = snprintf(messageBuffer, bufferSize, "|");

    //Encode the number of neurons
    offset += snprintf(messageBuffer+offset, bufferSize-offset, "%hhu ",nNeurons);

    // Encode the IDs of neurons whose outputs should be sent to specific IP addresses
    for (uint8_t i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",outputNeuronIds[i]);
    }

    //Encode the number of node IPs
    offset += snprintf(messageBuffer + offset, bufferSize-offset, "%hhu ",nNodes);

    // Encode the target IP addresses for the outputs of the specified neuron IDs
    for (uint8_t i = 0; i < nNodes; i++) {
        if(i != nNodes -1)offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu ",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
        else offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
    }
}

void NeuralNetworkCoordinator::encodePubSubInfo(char* messageBuffer, size_t bufferSize, uint8_t * neuronIDs, uint8_t nNeurons, int8_t subTopic, int8_t pubTopic){
    int offset = 0;
    // |[Number of Neurons] [neuron ID1] [neuron ID2] [Subscription 1] [Pub 1]

    offset = snprintf(messageBuffer, bufferSize, "|");

    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",nNeurons);

    // Encode the IDs of neurons that the Pub/sub info is about
    for (uint8_t i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",neuronIDs[i]);
    }

    // Encode the total number of subscriptions
    //offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",nSubTopics);

    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d ",subTopic);

    // Encode the total number of published topics
    //offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu ",nPubTopics);

    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%d",pubTopic);
}

void NeuralNetworkCoordinator::encodeForwardMessage(char*messageBuffer, size_t bufferSize, int inferenceId){
    snprintf(messageBuffer, bufferSize, "%d %i",NN_FORWARD,inferenceId);
}


void NeuralNetworkCoordinator::encodeInputAssignMessage(char*messageBuffer,size_t bufferSize,uint8_t neuronId){
    //NN_ASSIGN_INPUT [neuronID]
    snprintf(messageBuffer, bufferSize, "%d %hhu",NN_ASSIGN_INPUT,neuronId);
}


void NeuralNetworkCoordinator::handleACKMessage(char* messageBuffer){
    // NN_ACK [Acknowledge Neuron Id 1] [Acknowledge Neuron Id 2] [Acknowledge Neuron Id 3] ...
    char *saveptr1, *token;
    NeuronId currentId;
    NeuronEntry *neuronEntry;
    bool isNetworkAcknowledge = true;

    token = strtok_r(messageBuffer, " ",&saveptr1);

    //Skip the message header
    token = strtok_r(NULL, " ",&saveptr1);

    while(token != nullptr){
        // Extract the ID of the neuron
        currentId = atoi(token);

        //LOG(APP,DEBUG,"Acknowledged neuron Id: %hhu\n",currentId);
        neuronEntry = (NeuronEntry*)tableRead(neuronToNodeTable,&currentId);
        if(neuronEntry != nullptr){
            neuronEntry->isAcknowledged = true;
        }
        token = strtok_r(NULL, " ",&saveptr1);
    }

    //Check if with the received ACK all neurons have been acknowledged
    for (int i = 0; i < neuronToNodeTable->numberOfItems; i++) {
        neuronEntry = (NeuronEntry*)tableValueAtIndex(neuronToNodeTable,i);
        if(neuronEntry != nullptr){
            isNetworkAcknowledge = neuronEntry->isAcknowledged && isNetworkAcknowledge;
        }
    }

    receivedAllNeuronAcks = isNetworkAcknowledge;

}

void NeuralNetworkCoordinator::handleWorkerRegistration(char* messageBuffer){
    uint8_t nodeIP[4],deviceClass;

    //NN_WORKER_REGISTRATION [Node IP] [Device Type]
    sscanf(messageBuffer, "%*d %hhu.%hhu.%hhu.%hhu %hhu",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&deviceClass);

    if(totalWorkers>=TOTAL_NEURONS){
        LOG(APP,INFO, "Worker node registrations have exceeded the number of available NN neurons\n");
        return;
    }

    //Register the node as a worker node with the corresponding device class
    assignIP(workersIPs[totalWorkers],nodeIP);
    workersDeviceTypes[totalWorkers]=deviceClass;

    if(static_cast<DeviceType>(deviceClass) == DeviceType::DEVICE_ESP8266){
        neuronsPerWorker[totalWorkers] = NODES_PER_ESP8266;
        //numESP8266Workers++;
    }else if(static_cast<DeviceType>(deviceClass) == DeviceType::DEVICE_ESP32){
        neuronsPerWorker[totalWorkers] = NODES_PER_ESP32;
        //numESP32Workers++;
    }else if(static_cast<DeviceType>(deviceClass) == DeviceType::DEVICE_RPI){
        neuronsPerWorker[totalWorkers] = NODES_PER_RPI;
        //numRPiWorkers++;
    }/******/
    totalWorkers++;
}

void NeuralNetworkCoordinator::handleInputRegistration(char* messageBuffer){
    uint8_t nodeIP[4],deviceClass;

    //NN_INPUT_REGISTRATION [Node IP] [Device Type]
    sscanf(messageBuffer, "%*d %hhu.%hhu.%hhu.%hhu %hhu",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&deviceClass);

    if(totalInputs>=TOTAL_INPUT_NEURONS){
        LOG(APP,INFO, "Input node registrations have exceeded the number of available NN input nodes\n");
        return;
    }

    //Register the node as a input node
    assignIP(inputsIPs[totalInputs],nodeIP);
    totalInputs++;

}


void NeuralNetworkCoordinator::manageNeuralNetwork(){
    unsigned long currentTime = getCurrentTime();
    uint8_t myIP[4];

    network.getNodeIP(myIP);

    /*** Verify three conditions before distribution:
        1. Sufficient physical devices exist in the network
        2. Enough input nodes are registered
        3. Neural network isn't already distributed across devices ***/
    if(totalWorkers>=MIN_WORKERS && totalInputs == TOTAL_INPUT_NEURONS && !areNeuronsAssigned){
        LOG(APP,INFO,"Neural network distribution process started\n");

        LOG(APP,INFO,"Distributing input neurons\n");

        // Assign the input layer neurons to the input devices
        distributeInputNeurons(inputsIPs,totalInputs);

        LOG(APP,INFO,"Distributing hidden layer neurons\n");

        // Distribute the NN hidden layers to the available worker devices
        distributeNeuralNetworkBalanced(&neuralNetwork,workersIPs,totalWorkers,neuronsPerWorker);

        LOG(APP,INFO,"Distributing output neurons\n");

        //Assign the output layer neurons to a node(myself)
        distributeOutputNeurons(&neuralNetwork,myIP);

        LOG(APP,INFO,"Assigning Output targets\n");

        if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY){
            //Assign the output targets of the assigned neurons
            for (int i = 0; i < totalWorkers; i++) {
                assignOutputTargetsToNode(appPayload, sizeof(appPayload),workersIPs[i]);
                //network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,workersIPs[i]);
            }

        }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
            //Assign the pub/sub info of the assigned neurons
            for (int i = 0; i < totalWorkers; i++) {
                assignPubSubInfoToNode(appPayload, sizeof(appPayload),workersIPs[i]);
                //network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,workersIPs[i]);
            }
        }
        tablePrint(neuronToNodeTable,printNeuronTableHeader, printNeuronEntry);
        neuronAssignmentTime = getCurrentTime();
    }

    currentTime = getCurrentTime();
    /*** Node Assignment Verification:
        1. Check if nodes have existing assignments
        2. Verify not all ACKs for these assignments have arrived
        3. Confirm TIME_OUT period has elapsed since assignment
        If all conditions are met: Resend assignments to neurons with missing ACKs ***/
    if( areNeuronsAssigned && !receivedAllNeuronAcks && (currentTime-neuronAssignmentTime) >= ACK_TIMEOUT ){
        LOG(APP,INFO,"Missing ACKs detected, starting onACK timeout process (currentTime-neuronAssignmentTime)%lu:\n",(currentTime-neuronAssignmentTime));
        onACKTimeOut(workersIPs,totalWorkers);// Handles missing acknowledgment messages from hidden layer nodes
        onACKTimeOutInputLayer();// Handles missing acknowledgment messages from input layer neurons
        neuronAssignmentTime = getCurrentTime();
    }

    // If all neurons have acknowledged and no inference cycle is currently running, start a new inference cycle
    if(!inferenceRunning && receivedAllNeuronAcks){
        LOG(APP,INFO,"Starting an inference cycle\n");
        nnSequenceNumber +=2;
        encodeForwardMessage(appPayload, sizeof(appPayload),nnSequenceNumber);
        network.broadcastMessage(appBuffer,sizeof(appBuffer),appPayload);
        inferenceStartTime=getCurrentTime();
        inferenceRunning=true;/******/
    }

    currentTime = getCurrentTime();
    // If an inference cycle is running but exceeds the timeout period without results, start a new inference cycle
    if(inferenceRunning && (currentTime-inferenceStartTime) >= INFERENCE_TIMEOUT ){
        //TODO something where
    }

}

void NeuralNetworkCoordinator::onACKTimeOut(uint8_t nodeIP[][4],uint8_t nDevices){
    NeuronId *currentId;
    NeuronEntry *neuronEntry;
    uint8_t currentLayerIndex, currentIndexInLayer,*inputIndexMap;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    int i=0,messageOffset=0;
    bool unACKNeurons = false;

    /*** First iterate over physical devices that have been assigned neurons, since messages are aggregated and sent
     * per device (not per neuron). That is, if a device is assigned multiple neurons, the assignments are sent
     * together in a single message, not in separate messages. ***/
    for (uint8_t k = 0; k < nDevices; k++){
        // Encode the message header for the first node’s neuron assignments
        encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
        messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);

        /***
         * Aggregates all unacknowledged neurons from the same nodeIP. Since the message that assigns neurons to nodes
         * is sent as a single, aggregated message per node (i.e., it includes all the neurons the node is expected
         * to compute), if that message is lost, none of the neurons assigned to that node are acknowledged.
        ***/
        while(i <neuronToNodeTable->numberOfItems){
            currentId = (NeuronId*)tableKey(neuronToNodeTable,i);
            neuronEntry = (NeuronEntry*)tableRead(neuronToNodeTable,currentId);

            /***
             * If the neuron was not acknowledged by the node and it belongs to the current physical device along
             * with other unacknowledged neurons, then include it in the assigning message together with the others.
             ***/
            if(neuronEntry != nullptr && isIPEqual(nodeIP[k],neuronEntry->nodeIP) && !neuronEntry->isAcknowledged && neuronEntry->layer!=0){
                // The current layer index needs to be normalized because the neuronToNode table uses 0 for the input
                // layer, while in the NN structure, index 0 corresponds to the first hidden layer.
                currentLayerIndex = neuronEntry->layer - 1;
                currentIndexInLayer = neuronEntry->indexInLayer;
                unACKNeurons = true;

                //Remake the part of the message that maps the inputs into the input vector
                inputIndexMap = new uint8_t [neuralNetwork.layers[currentLayerIndex].numInputs];
                for (uint8_t j = 0; j < neuralNetwork.layers[currentLayerIndex].numInputs ; j++){
                    inputIndexMap[j] = *currentId+(j-neuralNetwork.layers[currentLayerIndex].numInputs);
                }

                //If the neuron is not from the input layer

                //Encode the part assigning neuron information (weights, bias, inputs etc..)
                encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                          *currentId,neuralNetwork.layers[currentLayerIndex].numInputs,inputIndexMap,
                                          &neuralNetwork.layers[currentLayerIndex].weights[currentIndexInLayer * neuralNetwork.layers[currentLayerIndex].numInputs],
                                          neuralNetwork.layers[currentLayerIndex].biases[currentIndexInLayer]);

                messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset,"%s",tmpBuffer);

                delete [] inputIndexMap;
            }
            i++;
        }

        //Send the message to the node
        if (unACKNeurons){
            //Send the message assigning weights bias and inputs
            network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodeIP[k]);

            if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY) {
                //Then send the message assigning the output targets
                assignOutputTargetsToNode(appPayload, sizeof(appPayload),nodeIP[k]);
            }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                assignPubSubInfoToNode(appPayload, sizeof(appPayload),nodeIP[k]);
            }
            //network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodeIP[k]);
        }

        //Reset the variables for the next unacknowledged neurons from other physical node
        messageOffset = 0;
        unACKNeurons = false;
        i=0;
    }
}

void NeuralNetworkCoordinator::onACKTimeOutInputLayer(){
    NeuronId *currentId;
    NeuronEntry *neuronEntry;
    int i=0;

    /*** First iterate over physical devices that have been assigned neurons, since messages are aggregated and sent
     * per device (not per neuron). That is, if a device is assigned multiple neurons, the assignments are sent
     * together in a single message, not in separate messages. ***/
    for (i=0;i<neuronToNodeTable->numberOfItems; i++){
        currentId = (NeuronId*)tableKey(neuronToNodeTable,i);
        neuronEntry = (NeuronEntry*)tableRead(neuronToNodeTable,currentId);
        // If a input neuron was not acknowledged then send the message to the node responsible for computing it
        if(neuronEntry != nullptr && !neuronEntry->isAcknowledged && neuronEntry->layer==0){

            //Encode the message assigning the input neuron to the node
            encodeInputAssignMessage(appPayload, sizeof(appPayload),*currentId);
            network.sendMessageToNode(appBuffer,sizeof(appBuffer),appPayload,neuronEntry->nodeIP);

            strcpy(appPayload,"");
            strcpy(appBuffer,"");

            if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY) {
                // Then send the message assigning output targets only to the specific input node
                assignOutputTargetsToNeurons(appPayload, sizeof(appPayload),currentId,1,neuronEntry->nodeIP);
            }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                //Then encode the message assigning pub/sub info only to the specific input node
                assignPubSubInfoToNeuron(appPayload, sizeof(appPayload),*currentId);
            }
            //network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,neuronEntry->nodeIP);
        }
    }
}

void NeuralNetworkCoordinator::handleNeuralNetworkMessage(uint8_t *senderIP, uint8_t *destinationIP, char *messageBuffer) {
    NeuralNetworkMessageType type;

    sscanf(messageBuffer, "%d",&type);
    switch (type) {
        case NN_WORKER_REGISTRATION:
            LOG(APP,INFO,"Received [NN_WORKER_REGISTRATION] message: \"%s\" from sender %hhu.%hhu.%hhu.%hhu\n"
                ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleWorkerRegistration(messageBuffer);
            break;

        case NN_INPUT_REGISTRATION:
            LOG(APP,INFO,"Received [NN_INPUT_REGISTRATION] message: \"%s\" from sender %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleInputRegistration(messageBuffer);
            break;

        case NN_ACK:
            LOG(APP,INFO,"Received [NN_ACK] message: \"%s\" from sender %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleACKMessage(messageBuffer);
            break;

        case NN_ASSIGN_COMPUTATION:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;

        case NN_ASSIGN_INPUT:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;

        case NN_ASSIGN_OUTPUT:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;

        case NN_ASSIGN_OUTPUT_TARGETS:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;

        case NN_NEURON_OUTPUT:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;
        case NN_FORWARD:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;
        case NN_NACK:
            handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            break;

        default:
            break;
    }
}



