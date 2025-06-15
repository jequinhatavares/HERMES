#include "strategy_topology.h"

//TODO Problemas: todos os nós tem uma tabela de métricas inicializada na memória
// TODO Talvez retirar o endereço da raiz da mensagem já que a mensagem PLA é sempre para a raíz

Strategy strategyTopology = {
        .handleMessage = handleMessageStrategyTopology,
        .encodeMessage = encodeMessageStrategyTopology,
        .influenceRouting = influenceRoutingStrategyTopology,
        .onTimer = onTimerStrategyTopology,
        .onNetworkEvent = onNetworkEventStrategyTopology,
        .getContext = getContextStrategyTopology,
};

uint8_t tmpChildIP[4];
uint8_t tmpChildSTAIP[4];

unsigned long lastMiddlewareUpdateTimeTopology;

bool hasTopologyMetric = false;

/***
 * Topology metrics table
 *
 * tTable[TableMaxSize] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the metrics table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the topology metrics table, including:
 * * * .numberOfItems - The current number of entries in the metrics table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the mTable.
 *
 * topologyMetricsTable - A pointer to TTable, used for accessing the topology metrics table.
 *
 * nodesTopology[TableMaxSize][4] - Preallocated memory for storing the IP addresses of the nodes.
 ***/

TableEntry tTable[TableMaxSize];
TableInfo TTable = {
        .numberOfItems = 0,
        .isEqual = isIPEqual,
        .table = tTable,
        .setKey = setIP,
        .setValue = nullptr,
};
TableInfo* topologyMetricsTable = &TTable;
uint8_t nodesTopology[TableMaxSize][4];

//Init Function Pointers
void (*encodeTopologyMetricValue)(char*,size_t,void *) = nullptr;
void (*decodeTopologyMetricValue)(char*,void *) = nullptr;

uint8_t * (*chooseParentFunction)(uint8_t *, uint8_t (*)[4] , uint8_t) = nullptr;


TopologyContext topologyContext ={
        .setMetric = topologySetNodeMetric,
};

void initStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *), uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t)){
    //Only the root initializes the table data; that is, only the root maintains a table containing the node metrics used to select parent nodes
    //if(!iamRoot) return;
    topologyMetricsTable->setValue = setValueFunction;

    tableInit(topologyMetricsTable, nodesTopology, topologyMetricValues, sizeof(uint8_t [4]), topologyMetricStructSize);

    encodeTopologyMetricValue = encodeTopologyMetricFunction;
    decodeTopologyMetricValue = decodeTopologyMetricFunction;
    chooseParentFunction = selectParentFunction;
}

void encodeMessageStrategyTopology(char* messageBuffer, size_t bufferSize, int typeTopology){

    /***if(typeTopology == TOP_PARENT_LIST_ADVERTISEMENT){
        //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
        offset = snprintf(messageBuffer, bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT,rootIP[0],rootIP[1],rootIP[2],rootIP[3],
                myIP[0],myIP[1],myIP[2],myIP[3]);

        for (int i = 0; i < reachableNetworks.len; i++) {
            sscanf(reachableNetworks.item[i], "JessicaNode%i:%i:%i:%i:%i:%i",&MAC[0],&MAC[1],&MAC[2],&MAC[3],&MAC[4],&MAC[5]);
            getIPFromMAC(MAC, nodeIP);

            offset += snprintf(messageBuffer + offset, bufferSize - offset," %i.%i.%i.%i",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
        }

    }else if(typeTopology == TOP_PARENT_ASSIGNMENT_COMMAND){
        //MESSAGE_TYPE TOP_PARENT_ASSIGNMENT_COMMAND [tmpParentIP] [nodeIP] [parentIP]
        snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_ASSIGNMENT_COMMAND,orphanIP[0],orphanIP[1],orphanIP[2],orphanIP[3],
                 newParentIP[0],newParentIP[1],newParentIP[2],newParentIP[3]);
    }***/
}

void encodeNodeMetricReport(char* messageBuffer, size_t bufferSize, void* metric){
    char metricBuffer[20];

    //Encode the metric value
    encodeTopologyMetricValue(metricBuffer, sizeof(metricBuffer),metric);

    //MESSAGE_TYPE TOP_METRICS_REPORT [destination:rootIP] [nodeIP] [metric]
    snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT,
             rootIP[0],rootIP[1],rootIP[2],rootIP[3],myIP[0],myIP[1],myIP[2],myIP[3],metricBuffer);
}
void encodeParentAssignmentCommand(char* messageBuffer, size_t bufferSize, uint8_t * destinationIP, uint8_t * chosenParentIP, uint8_t * targetNodeIP){
    //MESSAGE_TYPE TOP_PARENT_ASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",MIDDLEWARE_MESSAGE,TOP_PARENT_ASSIGNMENT_COMMAND
             ,destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],targetNodeIP[0],targetNodeIP[1],targetNodeIP[2],targetNodeIP[3]
             ,chosenParentIP[0],chosenParentIP[1],chosenParentIP[2],chosenParentIP[3]);
}
void encodeParentListAdvertisementRequest(char* messageBuffer, size_t bufferSize, parentInfo* possibleParents, int nrOfPossibleParents, uint8_t *temporaryParent, uint8_t *mySTAIP){
    int offset;

    /*** This is an intermediary message sent to the temporary parent.
         Since the node is not yet integrated into the network and doesn't know the root IP, the destination field is set to the temporary parent’s IP.
         The temporary parent will replace it with the correct root IP before forwarding.
         New Node-> Temporary Parent -> Root Node  ***/

    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    offset = snprintf(messageBuffer, bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT_REQUEST
                      ,temporaryParent[0],temporaryParent[1],temporaryParent[2],temporaryParent[3],
                      mySTAIP[0],mySTAIP[1],mySTAIP[2],mySTAIP[3],myIP[0],myIP[1],myIP[2],myIP[3]);

    for (int i = 0; i < nrOfPossibleParents; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset," %hhu.%hhu.%hhu.%hhu",
                           possibleParents[i].parentIP[0],possibleParents[i].parentIP[1],possibleParents[i].parentIP[2],possibleParents[i].parentIP[3]);
    }
}

void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize){
    TopologyMessageType type;
    uint8_t destinationNodeIP[4],*nextHopIP,targetNodeIP[4];
    int nChars = 0;

    //Extract Inject Message Types
    sscanf(messageBuffer,"%*i %i",&type);

    if(type == TOP_PARENT_LIST_ADVERTISEMENT_REQUEST){
        LOG(MESSAGES,INFO,"Received [PARENT_LIST_ADVERTISEMENT_REQUEST] message: \"%s\"\n", messageBuffer);
        sscanf(messageBuffer,"%*d %*d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3]
               ,&tmpChildSTAIP[0],&tmpChildSTAIP[1],&tmpChildSTAIP[2],&tmpChildSTAIP[3]
               ,&tmpChildIP[0],&tmpChildIP[1],&tmpChildIP[2],&tmpChildIP[3],&nChars);

        // Check whether this node is the intended destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            if(!iamRoot){
                //Encode the TOP_PARENT_LIST_ADVERTISEMENT message to be sent to the root
                //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
                snprintf(largeSendBuffer, sizeof(largeSendBuffer),"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT,rootIP[0],rootIP[1],rootIP[2],rootIP[3],
                         myIP[0],myIP[1],myIP[2],myIP[3],tmpChildIP[0],tmpChildIP[1],tmpChildIP[2],tmpChildIP[3],messageBuffer+nChars);
                //Send the encode message to the root
                nextHopIP = findRouteToNode(rootIP);
                if(nextHopIP != nullptr){
                    sendMessage(nextHopIP,largeSendBuffer);
                }
            }else{
                // If the new node connects directly to the root and sends the TOP_PARENT_LIST_ADVERTISEMENT_REQUEST,
                // the root can process the list of advertised parents and select a parent immediately
                chooseParentStrategyTopology(messageBuffer);
            }

        }else{
            // This message type is intended to be sent only by the new node to its directly connected temporary parent
            LOG(NETWORK, ERROR, "❌ ERROR: This node should be the destination of the TOP_PARENT_LIST_ADVERTISEMENT_REQUEST message\n");
        }
    }
    else if(type == TOP_PARENT_LIST_ADVERTISEMENT){
        LOG(MESSAGES,INFO,"Received [PARENT_LIST_ADVERTISEMENT] message: \"%s\"\n", messageBuffer);
        sscanf(messageBuffer,"%*d %*d %hhu.%hhu.%hhu.%hhu %n",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3],&nChars);
        // Check if i am the final destination of the message
        if(isIPEqual(destinationNodeIP,myIP) && iamRoot){

            chooseParentStrategyTopology(messageBuffer);

        }else{ // If not, forward the message to the nextHop to the destination
            nextHopIP = findRouteToNode(destinationNodeIP);
            if (nextHopIP != nullptr){
                sendMessage(nextHopIP,messageBuffer);
            }
        }
    }else if(type == TOP_PARENT_ASSIGNMENT_COMMAND){
        LOG(MESSAGES,INFO,"Received [PARENT_REASSIGNMENT_COMMAND] message: \"%s\"\n", messageBuffer);

        //MESSAGE_TYPE TOP_PARENT_ASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
        sscanf(messageBuffer,"%*d %*d %hhu.%hhu.%hhu.%hhu",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3]);
        // Check if i am the final destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            // If this node receives a parent reassignment command concerning the temporary child node, it must forward the message to that child
            sscanf(messageBuffer,"%*d %*d %*u.%*u.%*u.%*u %hhu.%hhu.%hhu.%hhu",&targetNodeIP[0],&targetNodeIP[1],&targetNodeIP[2],&targetNodeIP[3]);
            if(isIPEqual(targetNodeIP,tmpChildIP)){
                //Change the message destination IP to the childIP
                //snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_ASSIGNMENT_COMMAND,tmpChildIP[0],tmpChildIP[1],tmpChildIP[2],tmpChildIP[3],tmpChildIP[0],tmpChildIP[1],tmpChildIP[2],tmpChildIP[3],);
                sendMessage(tmpChildSTAIP,messageBuffer);
            }
        }else{ // If not, forward the message to the nextHop to the destination
            nextHopIP = findRouteToNode(destinationNodeIP);
            if (nextHopIP != nullptr){
                sendMessage(nextHopIP,messageBuffer);
            }
        }
    }else if(type ==TOP_METRICS_REPORT){
        //MESSAGE_TYPE TOP_METRICS_REPORT [destination:rootIP] [nodeIP] [metric]
        if(!iamRoot){// If the node is not the root, the metrics message is not intended for it
            nextHopIP = findRouteToNode(rootIP);
            if(nextHopIP != nullptr){
                sendMessage(nextHopIP,messageBuffer);
            }else{
                LOG(NETWORK,ERROR,"❌ERROR: No path to root node was found in the routing table.\n");
            }
        }else{
            sscanf(messageBuffer,"%*d %*d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n"
                   ,&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3],
                   &targetNodeIP[0],&targetNodeIP[1],&targetNodeIP[2],&targetNodeIP[3], &nChars);

            //Decode and register the metric in the table
            registerTopologyMetric(targetNodeIP,messageBuffer+nChars);
        }
    }
}
void onNetworkEventStrategyTopology(int networkEvent, uint8_t involvedIP[4]){
    void* metricValue;
    uint8_t *nextHopIP;
    switch (networkEvent) {
        case NETEVENT_JOINED_NETWORK:
            if(hasTopologyMetric && !iamRoot){//If the node already has an associated metric then send it to the root
                metricValue = tableRead(topologyMetricsTable,myIP);
                if(metricValue != nullptr){
                    encodeNodeMetricReport(smallSendBuffer, sizeof(smallSendBuffer),metricValue);
                    nextHopIP = findRouteToNode(rootIP);
                    if(nextHopIP != nullptr){
                        sendMessage(nextHopIP,smallSendBuffer);
                        LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/INJECT_NODE_INFO] message: \"%s\" to: %hhu.%hhu.%hhu.%hhu\n",smallSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);
                    }else{
                        LOG(NETWORK, ERROR, "❌ ERROR: No path to the root node (%hhu.%hhu.%hhu.%hhu) was found in the routing table.\n",rootIP[0],rootIP[1],rootIP[2],rootIP[3]);
                    }
                }
            }else{
                LOG(MIDDLEWARE,DEBUG,"did not sent any middleware metric\n");
            }
            break;

        default:
            break;
    }
}
void influenceRoutingStrategyTopology(char* dataMessage){
}
void onTimerStrategyTopology(){
    unsigned long currentTime = getCurrentTime();
    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTimeTopology) >= 10000 ) {
    }
}
void* getContextStrategyTopology(){
    return &topologyContext;
}

void registerTopologyMetric(uint8_t *nodeIP, char* metricBuffer){
    void *emptyEntry = nullptr;
    void *metricValue = tableRead(topologyMetricsTable,nodeIP);
    if(metricValue != nullptr){// If the nodeIP is already in the table update the corresponding metric value
        decodeTopologyMetricValue(metricBuffer,metricValue);
        tableUpdate(topologyMetricsTable,nodeIP,metricValue);
    }else {
        /*** If it is a new node, add the nodeIP to the table as a key with a corresponding dummy metric value (nullptr).
        This ensures that when the key is searched later, the function returns a pointer to a user-allocated struct.
        We cannot store the actual metric here because its structure is abstract and unknown to this layer.***/
        tableAdd(topologyMetricsTable, nodeIP, emptyEntry);
        metricValue = tableRead(topologyMetricsTable, nodeIP);
        decodeTopologyMetricValue(metricBuffer, metricValue);
        tableUpdate(topologyMetricsTable, nodeIP, metricValue);
    }
}

parentInfo requestParentFromRoot(parentInfo* possibleParents, int nrOfPossibleParents){
    uint8_t mySTAIP[4], temporaryParent[4],assignedParent[4];
    int packetSize=0, receivedMessageType;
    bool isExpectedMessage=false;

    LOG(MESSAGES,INFO,"Strategy Topology\n");


    // Select the first element of the possibleParents List to be the temporary parent
    if(nrOfPossibleParents != 0){
        connectToAP(possibleParents[0].ssid,PASS);
        assignIP(temporaryParent,possibleParents[0].parentIP);
        getMySTAIP(mySTAIP);
    }

    // Encode the PARENT_LIST_ADVERTISEMENT_REQUEST to send to the temporary parent, for in to send a PARENT_LIST_ADVERTISEMENT to the root
    encodeParentListAdvertisementRequest(largeSendBuffer, sizeof(largeSendBuffer), possibleParents, nrOfPossibleParents, temporaryParent, mySTAIP);

    // Send the message to the temporary parent so it can be forwarded toward the root
    sendMessage(temporaryParent,largeSendBuffer);

    LOG(MIDDLEWARE,INFO,"Sending %s to Temporary Parent: %hhu.%hhu.%hhu.%hhu\n",largeSendBuffer, temporaryParent[0],temporaryParent[1],temporaryParent[2],temporaryParent[3]);

    // Wait for the message from the root assigning me a parent
    unsigned long startTime = getCurrentTime();
    unsigned long currentTime = startTime;
    while( ((currentTime - startTime) <= 6000) && !isExpectedMessage ){
        packetSize = receiveMessage(receiveBuffer, sizeof(receiveBuffer));
        currentTime = getCurrentTime();
        if(packetSize>0){
            sscanf(receiveBuffer, "%*d %d",&receivedMessageType);
            if(receivedMessageType == TOP_PARENT_ASSIGNMENT_COMMAND){
                isExpectedMessage = true;
            }
        }
    }


    //Disconnect from the temporary parent
    disconnectFromAP();

    //Parse the received TOP_PARENT_ASSIGNMENT_COMMAND
    if(isExpectedMessage == true){
        LOG(MIDDLEWARE,INFO,"Received [PARENT_ASSIGNMENT_COMMAND]: %s\n",receiveBuffer);
        sscanf(receiveBuffer,"%*d %*d %*u.%*u.%*u.%*u %*u.%*u.%*u.%*u %hhu.%hhu.%hhu.%hhu",&assignedParent[0],&assignedParent[1],&assignedParent[2],&assignedParent[3]);
        // Search in the possibleParents the chosen one
        for (int i = 0; i < nrOfPossibleParents; i++) {
            if(isIPEqual(possibleParents[i].parentIP, assignedParent)){
                return possibleParents[i];
            }
        }
    }else{
        LOG(MIDDLEWARE,INFO,"Did not receive the [PARENT_ASSIGNMENT_COMMAND] message\n");
        return chooseParent(possibleParents,nrOfPossibleParents);
    }

    return possibleParents[0];

}

void chooseParentStrategyTopology(char* messageBuffer){
    uint8_t sourceIP[4], targetNodeIP[4],possibleParents[TableMaxSize][4],IP[4],*chosenParentIP = nullptr,*nextHopIP;
    uint8_t blankParent[4]={0,0,0,0};
    int middlewareMessageType,nChars=0,parentsCount=0;
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...

    sscanf(messageBuffer,"%*d %d %*u.%*u.%*u.%*u %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n",&middlewareMessageType,
            &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],&targetNodeIP[0],&targetNodeIP[1],&targetNodeIP[2],&targetNodeIP[3],&nChars);

    // Extract the list of possible parents from the message
    char* token = strtok(messageBuffer+nChars, " ");
    while (token != NULL) {
        sscanf(token,"%hhu.%hhu.%hhu.%hhu",&IP[0],&IP[1],&IP[2],&IP[3]);
        // Check if the nodeIP already exists in the middleware metrics table
        assignIP(possibleParents[parentsCount],IP);
        token = strtok(NULL, " ");
        parentsCount++;
    }

    //Select the node parent from the list of candidates using the application provided function
    if(chooseParentFunction == nullptr)LOG(MIDDLEWARE,ERROR,"ERROR: Choose Parent Function not initialized\n");
    else{
        chosenParentIP = chooseParentFunction(targetNodeIP,possibleParents,parentsCount);
    }

    //If the APP function did not find any suitable parent then return without sending the Parent Assign Command
    if(chosenParentIP == nullptr){
        LOG(MIDDLEWARE,DEBUG,"No parent chosen\n");
        return;
    }

    // Encode a TOP_PARENT_ASSIGNMENT_COMMAND to be sent either directly to the new node or to its temporary parent, who will relay the message
    if(middlewareMessageType == TOP_PARENT_LIST_ADVERTISEMENT_REQUEST){
        // If the message is of type TOP_PARENT_LIST_ADVERTISEMENT_REQUEST, it came directly from the new node we can send the new parent command to it directly
        encodeParentAssignmentCommand(smallSendBuffer,sizeof(smallSendBuffer), targetNodeIP, chosenParentIP,targetNodeIP);
        sendMessage(sourceIP,smallSendBuffer);
        LOG(MIDDLEWARE,INFO,"Sending %s to: %hhu.%hhu.%hhu.%hhu\n",smallSendBuffer, targetNodeIP[0],targetNodeIP[1],targetNodeIP[2],targetNodeIP[3]);

    }else if(middlewareMessageType == TOP_PARENT_LIST_ADVERTISEMENT){
        // If the received message is a TOP_PARENT_LIST_ADVERTISEMENT, it came from the temporary parent.
        //The parent assignment command must be sent to the temporary parent, who will relay it to the target node
        encodeParentAssignmentCommand(smallSendBuffer,sizeof(smallSendBuffer), sourceIP, chosenParentIP,targetNodeIP);
        nextHopIP = findRouteToNode(sourceIP);
        if(nextHopIP != nullptr){
            sendMessage(nextHopIP,smallSendBuffer);
        }
        LOG(MIDDLEWARE,INFO,"Sending %s to: %hhu.%hhu.%hhu.%hhu\n",smallSendBuffer, sourceIP[0],sourceIP[1],sourceIP[2],sourceIP[3]);

    }/******/
}

void topologySetNodeMetric(void* metric){
    uint8_t *nextHopIP;

    if(!iamRoot){ // If this node is not the root, send its metric data to the root for storage in the topologyMetricsTable.
        // Only send the metric message to the root if the node is already part of the network
        if(hasParent){
            encodeNodeMetricReport(smallSendBuffer,sizeof(smallSendBuffer),metric);
            nextHopIP = findRouteToNode(rootIP);
            if(nextHopIP != nullptr){
                sendMessage(nextHopIP,smallSendBuffer);
            }else{
                LOG(NETWORK,ERROR,"❌ERROR: No path to root node was found in the routing table.\n");
            }
        }

        //Add the node topology metric to the table
        void* tableEntry = tableRead(topologyMetricsTable,myIP);
        if(tableEntry == nullptr){ //The node is not yet in the table
            tableAdd(topologyMetricsTable, myIP, metric);
        }else{ //The node is already present in the table
            tableUpdate(topologyMetricsTable, myIP, metric);
        }

    }else{ // If the node is the root, it can directly store the metric in the topologyMetricsTable.
        void* tableEntry = tableRead(topologyMetricsTable,myIP);
        if(tableEntry == nullptr){ //The node is not yet in the table
            tableAdd(topologyMetricsTable, myIP, metric);
        }else{ //The node is already present in the table
            tableUpdate(topologyMetricsTable, myIP, metric);
        }
    }
    hasTopologyMetric = true;
}


void* getNodeTopologyMetric(uint8_t * nodeIP){
    return tableRead(topologyMetricsTable,nodeIP);
}

/******************************-----------Application Defined Functions----------------********************************/

uint8_t* chooseParentByProcessingCapacity(uint8_t * targetNodeIP, uint8_t potentialParents[][4], uint8_t nPotentialParents){
    int maxProcessingCapacity = 0;
    int bestParentIndex = -1;
    topologyTableEntry *topologyMetricValue;

    for (int i = 0; i < nPotentialParents; i++) {
        topologyMetricValue = (topologyTableEntry*) getNodeTopologyMetric(potentialParents[i]);
        if(topologyMetricValue != nullptr){
            LOG(MIDDLEWARE,DEBUG,"Potential Parent: %hhu.%hhu.%hhu.%hhu metric:%d\n",potentialParents[i][0],potentialParents[i][1],potentialParents[i][2],potentialParents[i][3],topologyMetricValue->processingCapacity);
            if(topologyMetricValue->processingCapacity >= maxProcessingCapacity){
                bestParentIndex = i;
                maxProcessingCapacity = topologyMetricValue->processingCapacity;
                LOG(MIDDLEWARE,DEBUG,"Parent Selected\n");
            }
        }
    }
    if(bestParentIndex != -1){
        LOG(MIDDLEWARE,DEBUG,"Chosen Parent: %hhu.%hhu.%hhu.%hhu metric:%d\n",potentialParents[bestParentIndex][0],potentialParents[bestParentIndex][1],potentialParents[bestParentIndex][2],potentialParents[bestParentIndex][3]);
        return potentialParents[bestParentIndex];
    }
    else{ return nullptr;}/******/
    return nullptr;
}

void encodeTopologyMetricEntry(char* buffer, size_t bufferSize, void *metricEntry){
    topologyTableEntry *metric = (topologyTableEntry*) metricEntry;
    snprintf(buffer,bufferSize,"%i", metric->processingCapacity);
}

void decodeTopologyMetricEntry(char* buffer, void *metricEntry){
    topologyTableEntry *metric = (topologyTableEntry*)metricEntry;
    sscanf(buffer,"%i", &metric->processingCapacity);
}


void setTopologyMetricValue(void* av, void*bv){
    if(bv == nullptr)return; //
    topologyTableEntry *a = (topologyTableEntry *) av;
    topologyTableEntry *b = (topologyTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}

void printTopologyMetricStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"Node[%hhu.%hhu.%hhu.%hhu] → (Topology Metric: %d) \n",
        ((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2],((uint8_t *)Table->key)[3],
        ((topologyTableEntry *)Table->value)->processingCapacity);
}