//#ifndef NEURAL_NET_IMPL
//#define NEURAL_NET_IMPL // Put the #define before the include to have access to the NN parameters
//#endif

#include "neural_network_coordinator.h"


#define NODES_PER_ESP8266 1
#define NODES_PER_ESP32 3
#define NODES_PER_RPI 5

unsigned long startAssignmentTime=0;


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


/***
 * NeuralNetworkCoordinator (Constructor)
 * Initializes the neural network coordinator by setting up the neuron-to-node mapping table
 ***/
NeuralNetworkCoordinator::NeuralNetworkCoordinator() {
    initNeuralNetwork();
}


/***
 * NeuralNetworkCoordinator (Destructor)
 * Frees the memory allocated in the coordinator class
 ***/
NeuralNetworkCoordinator::~NeuralNetworkCoordinator() {
    delete [] outputNeuronValues;
    delete [] isOutputReceived;
}


/**
 * initNeuralNetwork
 * Initializes the neuron-to-node mapping table for tracking neuron assignments.
 */
void NeuralNetworkCoordinator::initNeuralNetwork(){
    tableInit(neuronToNodeTable,neurons,neuronMap, sizeof(NeuronId),sizeof(NeuronEntry));
}

/**
 * printNeuronEntry
 * Function to Log a neuron entry in the neuron-to-node table.
 *
 * @param Table - Pointer to the table entry containing neuron information
 */
void printNeuronEntry(TableEntry* Table){
    LOG(APP,INFO,"Neuron ID %hhu → NodeIP[%hhu.%hhu.%hhu.%hhu] (Layer: %hhu) (Index in Layer: %hhu) (isAcknowledged: %d) \n",
         *(NeuronId*)Table->key,((NeuronEntry *)Table->value)->nodeIP[0],((NeuronEntry *)Table->value)->nodeIP[1]
        ,((NeuronEntry *)Table->value)->nodeIP[2],((NeuronEntry *)Table->value)->nodeIP[3],
        ((NeuronEntry *)Table->value)->layer,((NeuronEntry *)Table->value)->indexInLayer,
        ((NeuronEntry *)Table->value)->isAcknowledged);
}


/**
 * printNeuronTableHeader
 * Logs the header for the neuron-to-node table display.
 */
void printNeuronTableHeader(){
    LOG(APP,INFO,"((((((((((((((((((((((( Neuron To Node Table )))))))))))))))))))))))))))\n");
}


/**
 * distributeNeuralNetwork
 * Distributes neurons across worker nodes by dividing the total number of neurons by the number of computing devices,
 * ensuring a uniform allocation. Neurons are then assigned sequentially, layer by layer, across the network.
 * This function constructs and sends the assignment messages containing the neurons and their parameters.
 * The output layer is always assigned to the coordinator node.
 *
 * @param net - Pointer to the neural network structure
 * @param nodes - 2D array of worker nodes IP addresses
 * @param nrNodes - Number of available worker nodes
 */
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


/**
 * distributeNeuralNetworkBalanced
 * Distributes neurons across worker devices using device-specific capacity allocation.
 * Assigns neurons based on per-device neuron capacity (neuronsPerDevice array).
 * Processes layers sequentially, constructing aggregated assignment messages per device.
 * Output layer is not processed (handled separately by distributeOutputNeurons).
 *
 * @param net - Pointer to neural network structure
 * @param devices - 2D array of devices IP addresses
 * @param nrDevices - Number of available devices
 * @param neuronsPerDevice - Array specifying max neurons per device
 */
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
            // For subsequent hidden layers range from (currentNeuronId - (neuronsInPreviousLayer)) to (currentNeuronId)
            //if(i == 0)inputIndexMap[j] = j;
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
                if( (j != net->layers[i].numOutputs - 1) || (i != net->numLayers-2) ) assignedDevices ++;
                neuronPerNodeCount = 0;

                if(assignedDevices>=nrDevices){
                    LOG(APP, ERROR, "ERROR: The number of assigned devices exceeded the total available devices for neuron assignment.\n");
                    delete [] inputIndexMap;
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

/**
 * distributeNeuralNetworkBalanced
 * Distributes neurons across worker devices using device-specific capacity allocation.
 * Assigns neurons based on per-device neuron capacity (neuronsPerDevice array).
 * Processes layers sequentially, constructing aggregated assignment messages per device.
 * Output layer is not processed (handled separately by distributeOutputNeurons).
 *
 * This function ensures that assignments too large for a single payload are
 * divided and sent across multiple messages.
 *
 * @param net - Pointer to neural network structure
 * @param devices - 2D array of devices IP addresses
 * @param nrDevices - Number of available devices
 * @param neuronsPerDevice - Array specifying max neurons per device
 **/
void NeuralNetworkCoordinator::distributeNeuralNetworkBalancedV2(const NeuralNetwork *net,uint8_t devices[][4],uint8_t nrDevices,uint8_t neuronsPerDevice[]){
    uint8_t neuronPerNodeCount = 0, *inputIndexMap;
    int assignedDevices = 0, messageOffset = 0;
    uint8_t currentNeuronId = net->layers[0].numInputs; // first hidden neuron ID
    char tmpBuffer[150];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    NeuronEntry neuronEntry;
    bool singleDeviceMode = nrDevices == 1;

    auto resetPayloadWithHeader = [&](int &msgOffset) {
        msgOffset=0;
        msgOffset = encodeMessageHeader(appPayload, sizeof(appPayload), NN_ASSIGN_COMPUTATION);
    };

    // Start first message with the message header
    resetPayloadWithHeader(messageOffset);

    for (uint8_t i = 0; i < net->numLayers - 1; i++){
        /***Initialize the input index mapping before processing each layer. The inputIndexMapping specifies the order
        in which the node should store input values. It corresponds to an ordered list of neuron IDs from the previous
        layer, since those neurons serve as inputs to the current layer.***/
        inputIndexMap = new uint8_t[net->layers[i].numInputs];
        for (uint8_t j = 0; j < net->layers[i].numInputs; j++) {
            // For the first hidden layer, the input index mapping corresponds to the neuron IDs of the input layer (ranging from 0 to nrInputs - 1).
            // For subsequent hidden layers range from (currentNeuronId - (neuronsInPreviousLayer)) to (currentNeuronId)
            inputIndexMap[j] = currentNeuronId + (j - net->layers[i].numInputs);
        }

        for (uint8_t j = 0; j < net->layers[i].numOutputs; j++) {
            encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,currentNeuronId,net->layers[i].numInputs,
                                      inputIndexMap,&net->layers[i].weights[j * net->layers[i].numInputs],net->layers[i].biases[j]);

            /*** Before adding the assignment to the message buffer, check if it fits.
             *  If it doesn't fit, send the current encoded message to the node and
             *  start a new message with a fresh header. ***/
            int neededChars = snprintf(nullptr,0,"%s", tmpBuffer);
            if (neededChars < 0 || messageOffset + neededChars >= (int)(sizeof(appPayload))) {
                //Send the assignments to the node
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,devices[singleDeviceMode ? 0 : assignedDevices]);
                LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",appPayload,devices[singleDeviceMode ? 0 : assignedDevices][0],devices[singleDeviceMode ? 0 : assignedDevices][1],devices[singleDeviceMode ? 0 : assignedDevices][2],devices[singleDeviceMode ? 0 : assignedDevices][3],
                    messageOffset);
                // Encode the message header for the next neuron assignments
                resetPayloadWithHeader(messageOffset);
            }

            //Add to the current message the current node assignments
            messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset, "%s", tmpBuffer);

            // Add to mapping table the new assignments
            assignIP(neuronEntry.nodeIP, devices[singleDeviceMode ? 0 : assignedDevices]);
            neuronEntry.layer = i + 1;
            neuronEntry.indexInLayer = j;
            neuronEntry.isAcknowledged = false;
            tableAdd(neuronToNodeTable, &currentNeuronId, &neuronEntry);

            //Move on the next neuronId
            currentNeuronId++;
            neuronPerNodeCount++;

            bool lastNeuron = (i == net->numLayers - 2 && j == net->layers[i].numOutputs - 1);

            /***
            * When the number of neurons assigned to the current device reaches the expected amount, send the assignment
            * message and move on to the next device in the list. If the current device has fewer than the expected number
            * of neurons and no more neurons remain to be assigned, send the message to the node with the neurons already assigned.
            ***/
            if (!singleDeviceMode && (neuronPerNodeCount == neuronsPerDevice[assignedDevices] || lastNeuron)) {
                LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",appPayload,
                    devices[assignedDevices][0],devices[assignedDevices][1],devices[assignedDevices][2],devices[assignedDevices][3],messageOffset);
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload, devices[assignedDevices]);
                neuronPerNodeCount = 0;
                //Skip to the next device
                if (!lastNeuron) {
                    assignedDevices++;
                    if (assignedDevices >= nrDevices) {
                        LOG(APP, ERROR, "Too many devices assigned.\n");
                        delete[] inputIndexMap;
                        return;
                    }
                    resetPayloadWithHeader(messageOffset);
                }
            }

            // Single-device flush only happens on last neuron
            if (singleDeviceMode && lastNeuron) {
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload, devices[0]);
                LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",appPayload,devices[0][0],devices[0][1],devices[0][2],devices[0][3],messageOffset);
            }
        }
        delete[] inputIndexMap;
    }

    neuronAssignmentTime = getCurrentTime();
    areNeuronsAssigned = true;
}
/**
 * assignOutputTargetsToNetwork
 * Assigns output targets layer by layer across all nodes. Output targets are the devices that require the output
 * of neurons computed by other nodes. For each layer, identifies the devices responsible for computing the next-layer
 * neurons (those are the output targets). Constructs messages by grouping neurons in the same layer with their corresponding outputs and sends
 * these output target assignment messages to all worker nodes.
 *
 * @param nodes - 2D array of worker nodes IP addresses
 * @param nrNodes - Number of nodes in the network
 */
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
                    //LOG(APP,INFO,"IP of the next layer node: %hhu.%hhu.%hhu.%hhu \n",neuronEntry->nodeIP[0],neuronEntry->nodeIP[1],neuronEntry->nodeIP[2],neuronEntry->nodeIP[3]);
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
            encodeAssignOutputTargetsMessage(appPayload,sizeof(appPayload),outputNeurons,nNeurons,inputNodesIPs,nNodes);

            network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodes[j]);
            nNeurons = 0;
        }

        nNodes = 0;
    }

}


/**
 * assignOutputTargetsToNode
 * Assigns output targets for a specific device and its neurons.
 * For each layer containing the node's neurons, identifies the recipient nodes in the next layer.
 * Constructs a layer-wise message aggregating output target assignments for the node.
 *
 * This function ensures that assignments too large for a single payload are
 * divided and sent across multiple messages.
 *
 * @param messageBuffer - Buffer to store encoded message
 * @param bufferSize - Size of message buffer
 * @param targetNodeIP - IP address of target node
 */
void NeuralNetworkCoordinator::assignOutputTargetsToNode(char* messageBuffer,size_t bufferSize,uint8_t workerNodeIP[4]){
    NeuronId *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    uint8_t targetDevicesIPs[TABLE_MAX_SIZE][4], nTargetDevices = 0,myIP[4];
    int messageOffset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    bool neuronsAssignedToNode=false;

    network.getNodeIP(myIP);

    auto resetPayloadWithHeader = [&](int &msgOffset) {
        msgOffset=0;
        msgOffset = encodeMessageHeader(messageBuffer, sizeof(messageBuffer), NN_ASSIGN_OUTPUT_TARGETS);
    };

    //Start the new message
    resetPayloadWithHeader(messageOffset);

    /***  The messages in this function are constructed layer by layer. Messages are made based on aggregation of the
     * neurons computed in the same layer, since these neurons need to send their outputs to the same neurons or nodes
     * in the next layer. ***/
    for (uint8_t i = 0; i < neuralNetwork.numLayers+1; i++){

        /*** Identify the neurons computed by the target node at this layer and build
           *  a message with the IP addresses of the neurons that depend on its output.  ***/
        for (int k = 0; k < neuronToNodeTable->numberOfItems; ++k) {
            neuronId = (uint8_t*)tableKey(neuronToNodeTable,k);
            neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);

            if(neuronEntry != nullptr){
                //Search in the neuronToNodeTable for the neurons at a specific layer computed by the target node that have not been acknowledged
                if(isIPEqual(neuronEntry->nodeIP,workerNodeIP) && neuronEntry->layer == i && !neuronEntry->isAcknowledged){
                    //LOG(APP,INFO,"Neuron ID: %hhu \n",*neuronId);
                    outputNeurons[nNeurons] = *neuronId;
                    nNeurons ++;
                }
            }
        }

        //If the target node does not compute any neurons in the current layer skip to the next layer
        if(nNeurons==0){continue;}

        /*** The neurons in the next layer require the outputs of the current layer as input. Therefore, we need to
         * identify which nodes are responsible for computing the next layer's neurons. These are the nodes to which
         * the current layer's neurons must send their outputs. ***/
        for (int l = 0; l < neuronToNodeTable->numberOfItems; l++){
            neuronId = (uint8_t*)tableKey(neuronToNodeTable,l);
            neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable, neuronId);
            if(neuronEntry != nullptr){
                /*** Check these conditions:
                    1- Verify that the current device is responsible for computing a neuron in the next layer
                    2- Ensure the neuron is not already present in the list
                    3- Confirm the current device is not the same as the worker device (a device cannot have itself as a target)***/
                //LOG(APP,DEBUG,"IP of the node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry->nodeIP[0],neuronEntry->nodeIP[1],neuronEntry->nodeIP[2],neuronEntry->nodeIP[3]);
                //LOG(APP,DEBUG,"(neuronEntry->layer == i+1): %d !isIPinList(neuronEntry->nodeIP,targetDevicesIPs,nTargetDevices): %d !isIPEqual(neuronEntry->nodeIP,workerNodeIP): %d\n",(neuronEntry->layer == i+1),!isIPinList(neuronEntry->nodeIP,targetDevicesIPs,nTargetDevices),!isIPEqual(neuronEntry->nodeIP,workerNodeIP));
                //LOG(APP,DEBUG,"Neuron->layer: %d i+1:%d \n",neuronEntry->layer, i+1);
                if((neuronEntry->layer == i+1) && (!isIPinList(neuronEntry->nodeIP,targetDevicesIPs,nTargetDevices)) && (!isIPEqual(neuronEntry->nodeIP,workerNodeIP)) ){
                    assignIP(targetDevicesIPs[nTargetDevices],neuronEntry->nodeIP);
                    //LOG(APP,DEBUG,"IP of the next layer node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry->nodeIP[0],neuronEntry->nodeIP[1],neuronEntry->nodeIP[2],neuronEntry->nodeIP[3]);
                    nTargetDevices++;
                }
            }
        }


        //LOG(APP,DEBUG,"number of neurons: %hhu number of workerNodeIP: %hhu\n",nNeurons,nTargetDevices);
        // If the node computes any neurons in this layer.
        // Encode the message that includes its information along with the IPs to which it should forward the computation
        encodeAssignOutputTargetsMessage(tmpBuffer,tmpBufferSize,outputNeurons,nNeurons,targetDevicesIPs,nTargetDevices);

        /*** Before adding the assignment to the message buffer, check if it fits.
         *  If it doesn't fit, send the current encoded message to the node and
         *  start a new message with a fresh header. ***/
        int neededChars = snprintf(nullptr,0,"%s", tmpBuffer);
        if (neededChars < 0 || messageOffset + neededChars >= (int)(bufferSize)) {
            // If doesnt fit send the current encoded message as is
            network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,workerNodeIP);
            LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",messageBuffer
                ,workerNodeIP[0],workerNodeIP[1],workerNodeIP[2],workerNodeIP[3],messageOffset);
            // Encode the message header for the next neuron assignments
            resetPayloadWithHeader(messageOffset);
        }

        //Add to the current message the current node assignments
        messageOffset += snprintf(messageBuffer + messageOffset, bufferSize-messageOffset,"%s",tmpBuffer);
        neuronsAssignedToNode = true;


        nNeurons = 0;
        nTargetDevices = 0;
    }

    // Check if the target node has any assigned neurons. This safeguards against cases where the function is given a
    // targetIP with no neuron assignments, in such cases, there's no need to send a message.
    if(neuronsAssignedToNode){
        network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,workerNodeIP);
        LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",appPayload,workerNodeIP[0],workerNodeIP[1],workerNodeIP[2],workerNodeIP[3],messageOffset);

    }
}


/**
 * assignOutputTargetsToNeurons
 * Assigns output targets to the neurons computed by the target node.
 * For each neuron, identifies the recipient nodes in the next layer.
 * Constructs a message containing neuron-specific output targets and sends it.
 *
 * @param messageBuffer - Buffer to store encoded message
 * @param bufferSize - Size of message buffer
 * @param neuronIDs - Array of neuron IDs
 * @param nNeurons - Number of neurons in array
 * @param targetNodeIP - IP address of target node
 */
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
                //LOG(APP,INFO,"IP of the node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry2->nodeIP[0],neuronEntry2->nodeIP[1],neuronEntry2->nodeIP[2],neuronEntry2->nodeIP[3]);
                //LOG(APP,INFO,"!isIPinList(neuronEntry2->nodeIP,inputNodesIPs,nNodes): %d (neuronEntry2->layer == neuronEntry->layer+1): %d\n", !isIPinList(neuronEntry2->nodeIP,inputNodesIPs,nNodes),(neuronEntry2->layer == neuronEntry->layer+1));
                // Check if the current node is responsible for computing a neuron in the next layer and is not already in the list
                if(!isIPinList(neuronEntry2->nodeIP,inputNodesIPs,nNodes) && (neuronEntry2->layer == neuronEntry->layer+1)){
                    assignIP(inputNodesIPs[nNodes],neuronEntry2->nodeIP);
                    //LOG(APP,INFO,"IP of the next layer node: %hhu.%hhu.%hhu.%hhu\n",neuronEntry2->nodeIP[0],neuronEntry2->nodeIP[1],neuronEntry2->nodeIP[2],neuronEntry2->nodeIP[3]);
                    nNodes++;
                }
            }
        }

        encodeAssignOutputTargetsMessage(tmpBuffer,tmpBufferSize,neuronId,1,inputNodesIPs,nNodes);
        offset += snprintf(messageBuffer + offset, bufferSize-offset,"%s",tmpBuffer);

        nNodes = 0;
    }

    network.sendMessageToNode(appBuffer, sizeof(appBuffer),messageBuffer,targetNodeIP);

}


/**
 * distributeInputNeurons
 * Assigns input layer neurons to registered input producing devices.Sends individual assignment messages to each
 * input node. Adds neuron-node mappings to tracking table with unacknowledged status.
 *
 * @param inputNodes - 2D array of input node IP addresses
 * @param nrNodes - Number of available input nodes
 */
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


/**
 * distributeOutputNeurons
 * Assigns output layer neurons to a specific device.
 * Handles special case when coordinator node is the output device.
 * For remote devices: encodes and sends neuron assignment message.
 * For coordinator: directly configures neuron core and pub/sub information.
 *
 * This function ensures that assignments too large for a single payload are
 * divided and sent across multiple messages.
 *
 * @param net - Pointer to neural network structure
 * @param outputDevice - IP address of output device
 */
void NeuralNetworkCoordinator::distributeOutputNeurons(const NeuralNetwork *net,uint8_t outputDevice[4]){
    uint8_t outputLayer= net->numLayers - 1,*inputIndexMap;
    uint8_t myIP[4];
    network.getNodeIP(myIP);
    NeuronId currentOutputNeuron=0;
    NeuronEntry neuronEntry;
    char tmpBuffer[150];
    size_t tmpBufferSize= sizeof(tmpBuffer);
    int messageOffset=0,neuronStorageIndex=-1;

    outputNeuronValues = new float[net->layers[outputLayer].numOutputs];
    isOutputReceived = new bool[net->layers[outputLayer].numOutputs];
    for (int i = 0; i < net->layers[outputLayer].numOutputs; i++) {
        isOutputReceived[i] = false;
    }

    nOutputNeurons= net->layers[outputLayer].numOutputs;

    auto resetPayloadWithHeader = [&](int &msgOffset) {
        msgOffset=0;
        msgOffset = encodeMessageHeader(appPayload, sizeof(appPayload), NN_ASSIGN_OUTPUT);
    };

    //If the output device its not this node encode the message header
    if(!isIPEqual(outputDevice,myIP)){
        resetPayloadWithHeader(messageOffset);
    }

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

    // For each neuron in output layer
    for (uint8_t j = 0; j < net->layers[outputLayer].numOutputs; j++){

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

            //If the active strategy is the STRATEGY_PUBSUB save the topic that the output neuron publishes in the neuronToTopicMap
            if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                //Get where the current neuron was saved in neuron core
                neuronStorageIndex = neuronCore.getNeuronStorageIndex(currentOutputNeuron);
                // Record the topic published by this neuron in the neuron-to-topic map
                if(neuronStorageIndex != -1) neuronToTopicMap[neuronStorageIndex]= static_cast<int8_t>(outputLayer+1);
            }

            //Save the neuron in the list of Output Neurons handled by this device
            saveOutputNeuron(currentOutputNeuron);
        }else{
            // If this node doesn't compute the output layer, we must encode a message
            // assigning the output neurons and their parameters to the correct node.
            encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                      currentOutputNeuron,net->layers[outputLayer].numInputs,inputIndexMap,
                                      &net->layers[outputLayer].weights[j * net->layers[outputLayer].numInputs],net->layers[outputLayer].biases[j]);


            /*** Before adding the assignment to the message buffer, check if it fits.
             *  If it doesn't fit, send the current encoded message to the node and
             *  start a new message with a fresh header. ***/
            int neededChars = snprintf(nullptr,0,"%s", tmpBuffer);
            if (neededChars < 0 || messageOffset + neededChars >= (int)(sizeof(appPayload))) {
                // If doesnt fit send the current encoded message as is
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,outputDevice);
                LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",appPayload
                        ,outputDevice[0],outputDevice[1],outputDevice[2],outputDevice[3],messageOffset);
                // Encode the message header for the next neuron assignments
                resetPayloadWithHeader(messageOffset);
            }

            //Add to the current message the current node assignments
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
        if(network.getActiveMiddlewareStrategy() == STRATEGY_PUBSUB){
            //Then encode the message assigning pub/sub info only to the specific input node
            int8_t subTopic[1]={static_cast<int8_t>(outputLayer)},pubTopic[1]={static_cast<int8_t>(outputLayer+1)};
            network.subscribeAndPublishTopics(subTopic,1,pubTopic,1);
        }

    }

    delete [] inputIndexMap;
}


/**
 * assignPubSubInfoToNode
 * Assigns publish/subscribe information to a specific node for all its unacknowledged neurons.
 * Messages are constructed layer by layer, aggregating neurons from the same layer.
 * Handles message fragmentation when payload exceeds buffer capacity.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param targetNodeIP - IP address of the target node
 * @return void
 */
void NeuralNetworkCoordinator::assignPubSubInfoToNode(char* messageBuffer,size_t bufferSize,uint8_t targetNodeIP[4]){
    uint8_t *neuronId;
    NeuronEntry *neuronEntry;
    uint8_t outputNeurons[TOTAL_NEURONS], nNeurons = 0;
    int messageOffset = 0;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    bool neuronsAssignedToNode=false;

    auto resetPayloadWithHeader = [&](int &msgOffset) {
        msgOffset=0;
        msgOffset = encodeMessageHeader(appPayload, sizeof(appPayload), NN_ASSIGN_OUTPUT_TARGETS);
    };

    //Encode the message header
    resetPayloadWithHeader(messageOffset);

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
            /*** Neurons in a given layer will publish a topic corresponding to their layer number,
                and subscribe to the topic published by neurons in the previous layer.***/
            encodePubSubInfo(tmpBuffer,tmpBufferSize,outputNeurons,nNeurons,(int8_t)(i-1),i);

            /*** Before adding the assignment to the message buffer, check if it fits.
             *  If it doesn't fit, send the current encoded message to the node and
             *  start a new message with a fresh header. ***/
            int neededChars = snprintf(nullptr,0,"%s", tmpBuffer);
            if (neededChars < 0 || messageOffset + neededChars >= (int)(sizeof(appPayload)) ) {
                // If doesnt fit send the current encoded message as is
                network.sendMessageToNode(appBuffer, sizeof(appBuffer), messageBuffer, targetNodeIP);
                LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n", messageBuffer, targetNodeIP[0],
                    targetNodeIP[1], targetNodeIP[2], targetNodeIP[3], messageOffset);
                // Encode the message header for the next neuron assignments
                resetPayloadWithHeader(messageOffset);
            }

            //Increment the encoded message with the new pub/sub info
            messageOffset += snprintf(messageBuffer + messageOffset, bufferSize-messageOffset,"%s",tmpBuffer);
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

/**
 * assignPubSubInfoToNeuron
 * Assigns publish/subscribe information for a specific neuron to its computing node.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param neuronId - ID of the target neuron
 * @return void
 */
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


/**
 * encodeMessageHeader
 * Encodes a message header for neural network protocol messages.
 *
 * @param messageBuffer - Buffer to store the encoded header
 * @param bufferSize - Size of the message buffer
 * @param type - Type of neural network message
 * @return int - Number of characters written to the buffer
 */
int NeuralNetworkCoordinator::encodeMessageHeader(char* messageBuffer, size_t bufferSize,NeuralNetworkMessageType type){
    if(type == NN_ASSIGN_COMPUTATION){
        return snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_COMPUTATION);
    }else if(type == NN_ASSIGN_OUTPUT){
        return snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_OUTPUT);
    }else if(type == NN_ASSIGN_OUTPUT_TARGETS){
        return snprintf(messageBuffer,bufferSize,"%i ",NN_ASSIGN_OUTPUT_TARGETS);
    }
    return 0;
}


/**
 * encodeAssignNeuronMessage
 * Encodes a NN_ASSIGN_COMPUTATION message with weights, bias, and input information.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param neuronId - ID of the neuron being assigned
 * @param inputSize - Number of inputs to the neuron
 * @param inputSaveOrder - Array specifying input ordering
 * @param weightsValues - Array of weight values
 * @param bias - Bias value for the neuron
 * @return int - Number of characters written to the buffer
 */
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


/***
 * encodeAssignOutputTargetsMessage
 * Encodes an NN_ASSIGN_OUTPUT_TARGETS message specifying where neuron outputs should be sent.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param outputNeuronIds - Array of neuron IDs whose outputs need routing
 * @param nNeurons - Number of output neurons
 * @param IPs - 2D array of target IP addresses
 * @param nNodes - Number of target nodes
 * @return void
 ***/
void NeuralNetworkCoordinator::encodeAssignOutputTargetsMessage(char* messageBuffer, size_t bufferSize, uint8_t * outputNeuronIds, uint8_t nNeurons, uint8_t IPs[][4], uint8_t nNodes){
    int offset = 0;
    //|[N Neurons] [neuron ID1] [neuron ID2] ...[N Nodes] [IP Address 1] [IP Address 2] ...
    offset = snprintf(messageBuffer, bufferSize, "|");

    //Encode the number of neurons
    offset += snprintf(messageBuffer+offset, bufferSize-offset,"%hhu ",nNeurons);

    // Encode the IDs of neurons whose outputs should be sent to specific IP addresses
    for (uint8_t i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%hhu ",outputNeuronIds[i]);
    }

    //Encode the number of node IPs
    offset += snprintf(messageBuffer + offset, bufferSize-offset,"%hhu ",nNodes);

    // Encode the target IP addresses for the outputs of the specified neuron IDs
    for (uint8_t i = 0; i < nNodes; i++){
        if(i != nNodes -1)offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu ",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
        else offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu",IPs[i][0],IPs[i][1],IPs[i][2],IPs[i][3]);
    }
}


/**
 * encodePubSubInfo
 * Encodes an NN_ASSIGN_OUTPUT_TARGETS message with publish/subscribe information for a set of neurons.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param neuronIDs - Array of neuron IDs
 * @param nNeurons - Number of neurons
 * @param subTopic - Topic to subscribe to (previous layer)
 * @param pubTopic - Topic to publish to (current layer)
 * @return void
 */
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


/**
 * encodeForwardMessage
 * Encodes a NN_FORWARD message to initiate inference.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param inferenceId - Unique identifier for the inference cycle
 * @return void
 */
void NeuralNetworkCoordinator::encodeForwardMessage(char*messageBuffer, size_t bufferSize, int inferenceId){
    snprintf(messageBuffer, bufferSize, "%d %i",NN_FORWARD,inferenceId);
}


/**
 * encodeInputAssignMessage
 * Encodes an NN_ASSIGN_INPUT message.
 *
 * @param messageBuffer - Buffer to store the encoded message
 * @param bufferSize - Size of the message buffer
 * @param neuronId - ID of the input neuron
 * @return void
 */
void NeuralNetworkCoordinator::encodeInputAssignMessage(char*messageBuffer,size_t bufferSize,uint8_t neuronId){
    //NN_ASSIGN_INPUT [neuronID]
    snprintf(messageBuffer, bufferSize, "%d %hhu",NN_ASSIGN_INPUT,neuronId);
}


/**
 * handleACKMessage
 * Processes acknowledgment messages from neural network nodes.
 * Updates acknowledgment status of neurons and checks if all neurons have acknowledged.
 *
 * @param messageBuffer - Received acknowledgment message
 * @return void
 */
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
    if(isNetworkAcknowledge){
        reportSetupTime(getCurrentTime()-startAssignmentTime);
        readyForInferenceTime=getCurrentTime();
    }

}

/**
 * handleWorkerRegistration
 * Decodes and handles NN_WORKER_REGISTRATION messages and assigns neuron computations based on the device type.
 *
 * @param messageBuffer - Buffer containing registration message.
 * Message Format: NN_WORKER_REGISTRATION [Node IP] [Device Type]
 */
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


/**
 * handleInputRegistration
 * Decodes an input registration message and registers devices that provide values for the neural network’s input layer
 *
 * @param messageBuffer - Buffer containing registration message
 * Message Format: NN_INPUT_REGISTRATION [Node IP] [Device Type]
 */
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

/**
 * handleOutputRegistration
 * Decodes an output registration message and registers devices that provide values for the neural network’s output layer
 *
 * @param messageBuffer - Buffer containing registration message
 * Message Format: NN_OUTPUT_REGISTRATION [Node IP]
 */
void NeuralNetworkCoordinator::handleOutputRegistration(char* messageBuffer){
    uint8_t nodeIP[4];

    //NN_INPUT_REGISTRATION [Node IP] [Device Type]
    sscanf(messageBuffer, "%*d %hhu.%hhu.%hhu.%hhu",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3]);

    if(nrOutputDevices>=1){
        LOG(APP,INFO, "Output node registrations have exceeded the number of available NN output devices\n");
        return;
    }

    //Register the device as a output device
    assignIP(outputIPs[nrOutputDevices],nodeIP);
    nrOutputDevices++;
}


/**
 * manageNeuralNetwork
 * Orchestrates neural network operation:
 * 1- When sufficient worker and input nodes are registered, distributes neuron computations among them and assigns output targets.
 * 2- Tracks acknowledgments from all nodes to confirm assignment. And if not received within a timeout resends the assignments
 * 3- Once all ACKs are received, initiates an inference cycle.
 * @return void
 */
void NeuralNetworkCoordinator::manageNeuralNetwork(){
    unsigned long currentTime = getCurrentTime();
    uint8_t myIP[4];

    network.getNodeIP(myIP);

    /*** Verify three conditions before distribution:
        1. Sufficient physical devices exist in the network
        2. Enough input nodes are registered
        3. Neural network isn't already distributed across devices ***/
    if(totalWorkers>=MIN_WORKERS && totalInputs == TOTAL_INPUT_NEURONS && !areNeuronsAssigned){
        startAssignmentTime=getCurrentTime();

        LOG(APP,INFO,"Neural network distribution process started\n");

        LOG(APP,INFO,"Distributing input neurons\n");

        // Assign the input layer neurons to the input devices
        distributeInputNeurons(inputsIPs,totalInputs);

        LOG(APP,INFO,"Distributing hidden layer neurons\n");

        // Distribute the NN hidden layers to the available worker devices
        distributeNeuralNetworkBalancedV2(&neuralNetwork,workersIPs,totalWorkers,neuronsPerWorker);

        LOG(APP,INFO,"Distributing output neurons\n");

        // If an output device is registered, assign output neurons to that device.
        // Otherwise, the coordinator (myself) handles the output calculations.
        if(nrOutputDevices == 1) distributeOutputNeurons(&neuralNetwork,outputIPs[0]);
        else distributeOutputNeurons(&neuralNetwork,myIP);

        LOG(APP,INFO,"Assigning Output targets\n");

        if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY || network.getActiveMiddlewareStrategy()==STRATEGY_INJECT){
            //Assign the output targets of the assigned neurons
            for (int i = 0; i < totalWorkers; i++) {
                assignOutputTargetsToNode(appPayload, sizeof(appPayload),workersIPs[i]);
                //network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,workersIPs[i]);
            }
            for (int i = 0; i < totalInputs; ++i) {
                // Send the ASSIGN_OUTPUT_TARGETS message to devices responsible for input neurons, if it has not already been sent.
                if(!isIPinList(inputsIPs[i],workersIPs,totalWorkers)){
                    assignOutputTargetsToNode(appPayload, sizeof(appPayload),inputsIPs[i]);
                }
            }

        }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
            //Assign the pub/sub info of the assigned neurons
            for (int i = 0; i < totalWorkers; i++) {
                assignPubSubInfoToNode(appPayload, sizeof(appPayload),workersIPs[i]);
                //network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,workersIPs[i]);
            }
            for (int i = 0; i < totalInputs; ++i) {
                // Send the ASSIGN_OUTPUT_TARGETS message to devices responsible for input neurons, if it has not already been sent.
                if(!isIPinList(inputsIPs[i],workersIPs,totalWorkers)){
                    assignPubSubInfoToNode(appPayload, sizeof(appPayload),inputsIPs[i]);
                }
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

    currentTime = getCurrentTime();
    // If all ACKs have been received, wait for a set interval before starting a new inference cycle
    if(receivedAllNeuronAcks && (currentTime-readyForInferenceTime)>= INFERENCE_INTERVAL ){
        hasWaitedBeforeInference=true;
    }

    /*** Inference Cycle Initiation:
     * 1. Verify no inference cycle is currently active
     * 2. Confirm all neurons have acknowledged previous operations
     * 3. Ensure minimum interval between inference cycles has elapsed ***/
    if(!inferenceRunning && receivedAllNeuronAcks && hasWaitedBeforeInference){
        LOG(APP,INFO,"Starting an inference cycle\n");
        nnSequenceNumber +=2;
        //Clear the parameters related to the neurons the coordinator calculates
        clearNeuronInferenceParameters();
        encodeForwardMessage(appPayload, sizeof(appPayload),nnSequenceNumber);
        network.broadcastMessage(appBuffer,sizeof(appBuffer),appPayload);
        inferenceStartTime=getCurrentTime();
        //Reset the variables to don't start a new inference until the current one finishes
        inferenceRunning=true;/******/
        hasWaitedBeforeInference=false;
    }

    currentTime = getCurrentTime();
    // If an inference cycle is running but exceeds the timeout period without results, start a new inference cycle
    if(inferenceRunning && !inferenceComplete && (currentTime-inferenceStartTime) >= INFERENCE_TIMEOUT ){
        //TODO something where
    }

}

/**
 * onACKTimeOut
 * Manages hidden layer neuron assignment timeouts.
 * If a node fails to acknowledge its assignment, the system resends the assignment to that node.
 *
 * This function ensures that assignments too large for a single payload are
 * divided and sent across multiple messages.
 *
 * @param nodeIP - 2D array of worker nodes IP addresses
 * @param nDevices - Number of worker devices to process
 * @return void
 */
void NeuralNetworkCoordinator::onACKTimeOut(uint8_t nodeIP[][4],uint8_t nDevices){
    NeuronId *currentId;
    NeuronEntry *neuronEntry;
    uint8_t currentLayerIndex, currentIndexInLayer,*inputIndexMap;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    int i=0,messageOffset=0;
    bool unACKNeurons = false;

    auto resetPayloadWithHeader = [&](int &msgOffset) {
        msgOffset=0;
        msgOffset = encodeMessageHeader(appPayload, sizeof(appPayload), NN_ASSIGN_COMPUTATION);
    };

    /*** First iterate over physical devices that have been assigned neurons, since messages are aggregated and sent
     * per device (not per neuron). That is, if a device is assigned multiple neurons, the assignments are sent
     * together in a single message, not in separate messages. ***/
    for (uint8_t k = 0; k < nDevices; k++){
        // Encode the message header for the first node’s neuron assignments
        //encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
        //messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);
        resetPayloadWithHeader(messageOffset);

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


                //Encode the part assigning neuron information (weights, bias, inputs etc..)
                encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                          *currentId,neuralNetwork.layers[currentLayerIndex].numInputs,inputIndexMap,
                                          &neuralNetwork.layers[currentLayerIndex].weights[currentIndexInLayer * neuralNetwork.layers[currentLayerIndex].numInputs],
                                          neuralNetwork.layers[currentLayerIndex].biases[currentIndexInLayer]);


                /*** Before adding the assignment to the message buffer, check if it fits.
                 *  If it doesn't fit, send the current encoded message to the node and
                 *  start a new message with a fresh header. ***/
                int neededChars = snprintf(nullptr,0,"%s", tmpBuffer);
                if (neededChars < 0 || messageOffset + neededChars >= (int)(sizeof(appPayload))) {
                    // If doesnt fit send the current encoded message as is
                    network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodeIP[k]);
                    LOG(APP, DEBUG, "Message sent: %s to %hhu.%hhu.%hhu.%hhu Size:%d\n",appPayload
                            ,nodeIP[k][0],nodeIP[k][1],nodeIP[k][2],nodeIP[k][3],messageOffset);
                    // Encode the message header for the next neuron assignments
                    resetPayloadWithHeader(messageOffset);
                }

                //Add to the current message the current node assignments
                messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset,"%s",tmpBuffer);

                delete [] inputIndexMap;
            }
            i++;
        }

        //Send the message to the node
        if (unACKNeurons){
            //Send the message assigning weights bias and inputs
            network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,nodeIP[k]);

            if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY || network.getActiveMiddlewareStrategy()==STRATEGY_INJECT) {
                //Then send the message assigning the output targets
                assignOutputTargetsToNode(appPayload, sizeof(appPayload),nodeIP[k]);
            }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                //Then send the message assigning the pub sub info
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


/**
 * onACKTimeOutInputLayer
 * Handles input layer neuron assignment timeouts by resending unacknowledged assignments.
 *
 * @return void
 */
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

            if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY || network.getActiveMiddlewareStrategy()==STRATEGY_INJECT) {
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


/**
 * handleNeuralNetworkMessage
 * Main dispatcher for neural network protocol messages.
 *
 * @param senderIP - Source IP address
 * @param destinationIP - Destination IP address
 * @param messageBuffer - Received message payload buffer
 * @return void
 */
void NeuralNetworkCoordinator::handleNeuralNetworkMessage(uint8_t *senderIP, uint8_t *destinationIP, char *messageBuffer) {
    NeuralNetworkMessageType type;
    NeuronId neuronId;
    float neuronOutput;
    int receivedInferenceId;

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

        case NN_OUTPUT_REGISTRATION:
            LOG(APP,INFO,"Received [NN_OUTPUT_REGISTRATION] message: \"%s\" from sender %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleOutputRegistration(messageBuffer);
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
            sscanf(messageBuffer, "%*d %i %hhu %f",&receivedInferenceId,&neuronId,&neuronOutput);
            //If the received neuron is an output layer neuron and its from the current inference running save the value
            if(isOutputNeuron(neuronId) && nnSequenceNumber == receivedInferenceId){
                onNeuralNetworkOutput(neuronId,neuronOutput);
            }else{
                handleNeuronMessage(senderIP, destinationIP,messageBuffer);
            }
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

void NeuralNetworkCoordinator::onNeuralNetworkOutput(NeuronId neuronId, float outputValue) {
    bool allOutputNeuronsReceived=true;
    unsigned long currentTime=getCurrentTime();

    /*** Coordinator-side override of neuron worker output processing:
     * When the coordinator generates output neuron values, it locally aggregates results
     * instead of propagating them through the network. Once all output values are computed,
     * the inference cycle terminates and performance metrics are logged to the monitoring server. ***/
    LOG(APP,INFO,"/////// Inference Cycle Output - NeuronId:%hhu Output Value:%f //////////\n",neuronId,outputValue);

    NeuronEntry* neuronEntry= (NeuronEntry*) tableRead(neuronToNodeTable,&neuronId);
    if(!neuronEntry)return;

    // Mark the neuron output value as received
    if(!isOutputReceived[neuronEntry->indexInLayer]){
        outputNeuronValues[neuronEntry->indexInLayer] = outputValue;
        isOutputReceived[neuronEntry->indexInLayer]=true;
    }

    // Verify if all output neuron value have been received
    for (int i = 0; i < nOutputNeurons; ++i) {
        allOutputNeuronsReceived = allOutputNeuronsReceived && isOutputReceived[i];
        //LOG(APP,DEBUG,"isOutputReceived[i]=%d\n",isOutputReceived[i]);
    }
    //LOG(APP,DEBUG,"allOutputNeuronsReceived=%d\n",allOutputNeuronsReceived);

    //If all output neurons values have been received report the inference information to the monitoring server
    if(allOutputNeuronsReceived){
        reportInferenceResults(nnSequenceNumber,currentTime-inferenceStartTime,nackCount,outputNeuronValues,nOutputNeurons);
    }
}

bool NeuralNetworkCoordinator::isOutputNeuron(NeuronId neuronId) {
    uint8_t outputLayer= neuralNetwork.numLayers - 1;
    NeuronEntry* neuronEntry= (NeuronEntry*) tableRead(neuronToNodeTable,&neuronId);

    if(!neuronEntry)return false;
    if(neuronEntry->layer == outputLayer + 1) return true;
    return false;
}


void NeuralNetworkCoordinator::clearInferenceVariables(){
    for (int i = 0; i < nOutputNeurons; ++i) {
        isOutputReceived[i]=false;
        outputNeuronValues[i]=0;
    }
    inferenceRunning = false;
}






