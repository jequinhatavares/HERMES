#include "strategy_inject.h"

Strategy strategyInject = {
    .handleMessage = handleMessageStrategyInject,
    .encodeMessage = encodeMessageStrategyInject,
    .influenceRouting = influenceRoutingStrategyInject,
    .onTimer = onTimerStrategyInject,
    .onNetworkEvent = onNetworkEventStrategyInject,
    .getContext = getContextStrategyInject,
};


InjectContext injectContext ={
        .injectNodeMetric = injectNodeMetric,
};


void setIP(void* av, void* bv);


unsigned long lastMiddlewareUpdateTimeInject = 0;


//void (*setValue)(void*,void*) = nullptr;

/***
 * Middleware metrics table
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
 * nodeIP[TableMaxSize][4] - Preallocated memory for storing the IP addresses of the nodes.
 ***/
TableEntry mTable[TableMaxSize];
TableInfo MTable = {
        .numberOfItems = 0,
        .isEqual = isIPEqual,
        .table = mTable,
        .setKey = setKey,
        .setValue = nullptr,
};
TableInfo* metricsTable = &MTable;
uint8_t nodes[TableMaxSize][4];

//Function Pointers Initializers
void (*encodeMetricValue)(char*,size_t,void *) = nullptr;
void (*decodeMetricValue)(char*,void *) = nullptr;


/**
 * initStrategyInject
 * Initializes the Inject strategy by setting up function pointers and preparing the metrics table.
 *
 * @param setValueFunction - Function pointer to set a value in the metrics table.
 * @param metricStruct - Pointer to the memory where the metric structure values are stored.
 * @param metricStructSize - Size of the metric structure in bytes.
 * @param encodeMetricFunction - Function pointer to encode metric values into a buffer.
 * @param decodeMetricFunction - Function pointer to decode metric values from a buffer.
 *
 * @return void
 */
void initStrategyInject(void (*setValueFunction)(void*,void*), void *metricValues, size_t metricStructSize,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)) {
    metricsTable->setValue = setValueFunction;
    tableInit(metricsTable, nodes, metricValues, sizeof(uint8_t [4]), metricStructSize);

    encodeMetricValue = encodeMetricFunction;
    decodeMetricValue = decodeMetricFunction;

}

/**
 * injectNodeMetric
 * Inserts/updates the metric data for this node.
 *
 * @param metric - Pointer to the metric data to be injected or updated.
 * @return void
 */
void injectNodeMetric(void* metric){
    void*tableEntry = tableRead(metricsTable,myIP);
    if(tableEntry == nullptr){ //The node is not yet in the table
        tableAdd(metricsTable, myIP, metric);
    }else{ //The node is already present in the table
        tableUpdate(metricsTable, myIP, metric);
    }
}

/**
 * rewriteSenderIPInject
 * Updates the sender IP address of a message buffer before further propagation.
 *
 * @param messageBuffer - Pointer to the buffer containing the encoded message.
 * @param bufferSize - Size of the message buffer in bytes.
 * @param type - The type of Inject message being processed.
 *
 * @return void
 */
void rewriteSenderIPInject(char* messageBuffer, char* writeBuffer,size_t writeBufferSize, InjectMessageType type) {
    int messageType, nChars;
    uint8_t senderIP[4],nodeIP[4];
    InjectMessageType injectType;


    if(type == INJECT_NODE_INFO){
        // If the encoded message already contains metric information, it means this is a propagation of an already encoded message.
        // In this case, only the sender address needs to be updated before further propagation.
        if( sscanf(messageBuffer,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n",&messageType,&injectType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]
                ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &nChars) == 10 ){

            LOG(MIDDLEWARE,DEBUG,"1\n");
            snprintf(writeBuffer,writeBufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",messageType,injectType,myIP[0],myIP[1],myIP[2],myIP[3]
                    ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], messageBuffer + nChars);

        }
    }
}

/**
 * encodeMessageStrategyInject
 * Encodes strategy Inject messages of a given type.
 *
 *
 * @param messageBuffer - Pointer to the buffer where the encoded message will be written.
 * @param bufferSize - Size of the message buffer in bytes.
 * @param type - The Inject message type to be encoded (e.g., INJECT_NODE_INFO or INJECT_TABLE_INFO).
 *
 * @return void
 */
void encodeMessageStrategyInject(char* messageBuffer, size_t bufferSize, int type){
    int offset = 0;
    uint8_t *entryIP;
    char tmpBuffer[20], tmpBuffer2[50];
    void *metricValue;
    InjectMessageType injectType;

    if(type == INJECT_NODE_INFO){
        //Max Size: 20 + metric encoding size
        //MESSAGE_TYPE INJECT_NODE_INFO [sender IP] [nodeIP] metric
        encodeMyMetric(tmpBuffer, sizeof(tmpBuffer));
        snprintf(messageBuffer, bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %s",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO,myIP[0],myIP[1],myIP[2],myIP[3],tmpBuffer);

    }else if(type == INJECT_TABLE_INFO){//INJECT_TABLE_INFO
        //MESSAGE_TYPE INJECT_TABLE_INFO [sender IP] |[nodeIP] metric |[nodeIP] metric |...
        offset = snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu",MIDDLEWARE_MESSAGE,INJECT_TABLE_INFO,myIP[0],myIP[1],myIP[2],myIP[3]);

        for (int i = 0; i < metricsTable->numberOfItems; i++) {
            entryIP = (uint8_t *)tableKey(metricsTable,i);
            metricValue = tableRead(metricsTable,entryIP);
            if(metricValue != nullptr){
                encodeMetricValue(tmpBuffer, sizeof(tmpBuffer),metricValue);
                offset += snprintf(messageBuffer + offset, bufferSize - offset," |%hhu.%hhu.%hhu.%hhu %s",entryIP[0],entryIP[1],entryIP[2],entryIP[3],tmpBuffer);
            }
        }
    }

}

/**
 * encodeMyMetric
 * Encodes the current node's IP address and its corresponding metric into the provided message buffer.
 *
 * @param messageBuffer - Pointer to the buffer where the encoded message will be written.
 * @param bufferSize - Size of the message buffer in bytes.
 */
void encodeMyMetric(char* messageBuffer, size_t bufferSize){
    char tmpBuffer[20];

    snprintf(messageBuffer,bufferSize,"%hhu.%hhu.%hhu.%hhu ", myIP[0],myIP[1],myIP[2],myIP[3]);

    void* metricValue = tableRead(metricsTable, myIP);
    if(metricValue != nullptr){
        encodeMetricValue(tmpBuffer, sizeof(tmpBuffer),metricValue);
        strcat(messageBuffer, tmpBuffer);
    }
}

/**
 * handleMessageStrategyInject
 * Handles strategy Inject messages based on their type
 *
 *
 * @param messageBuffer - Pointer to the buffer containing the received message.
 * @param bufferSize - Size of the message buffer in bytes.
 *
 * @return void
 */
void handleMessageStrategyInject(char* messageBuffer, size_t bufferSize){
    char metricBuffer[20];
    uint8_t senderIP[4],nodeIP[4];
    void  *emptyEntry = nullptr;
    InjectMessageType injectType;

    //Extract Inject Message Types
    sscanf(messageBuffer,"%*i %i",&injectType);

    if(injectType == INJECT_NODE_INFO){
        //MESSAGE_TYPE INJECT_NODE_INFO [sender IP] [nodeIP] metric
        sscanf(messageBuffer,"%*d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",&injectType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
               &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);

        // Check if the nodeIP already exists in the middleware metrics table
        void *metricValue = tableRead(metricsTable,nodeIP);
        if(metricValue != nullptr){// If the nodeIP is already in the table update the corresponding metric value
            decodeMetricValue(metricBuffer,metricValue);
            tableUpdate(metricsTable,nodeIP,metricValue);
        }else{
            /*** If it is a new node, add the nodeIP to the table as a key with a corresponding dummy metric value (nullptr).
            This ensures that when the key is searched later, the function returns a pointer to a user-allocated struct.
            We cannot store the actual metric here because its structure is abstract and unknown to this layer.***/
            tableAdd(metricsTable,nodeIP,emptyEntry);
            metricValue = tableRead(metricsTable,nodeIP);
            decodeMetricValue(metricBuffer,metricValue);
            tableUpdate(metricsTable,nodeIP,metricValue);
        }
        //Print the updated table
        LOG(MESSAGES,INFO,"Updated Middleware Table\n");
        tablePrint(metricsTable,printMetricStruct);

        //Encode this node IP as the sender IP and propagate the message
        rewriteSenderIPInject(messageBuffer,smallSendBuffer, sizeof(smallSendBuffer),INJECT_NODE_INFO);
        propagateMessage(smallSendBuffer,senderIP);

    }else if(injectType == INJECT_TABLE_INFO){
        //MESSAGE_TYPE INJECT_TABLE_INFO [sender IP] |[nodeIP] metric |[nodeIP] metric |...
        char* token = strtok(messageBuffer, "|");
        //To discard the message type and ensure the token points to the first routing table update entry
        token = strtok(NULL, "|");
        while (token != NULL) {
            sscanf(token,"%hhu.%hhu.%hhu.%hhu %s",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);
            // Check if the nodeIP already exists in the middleware metrics table
            void *metricValue = tableRead(metricsTable,nodeIP);
            if(metricValue != nullptr){// If the nodeIP is already in the table update the corresponding metric value
                decodeMetricValue(metricBuffer,metricValue);
                tableUpdate(metricsTable,nodeIP,metricValue);
            }else{
                /*** If it is a new node, add the nodeIP to the table as a key with a corresponding dummy metric value (nullptr).
                This ensures that when the key is searched later, the function returns a pointer to a user-allocated struct.
                We cannot store the actual metric here because its structure is abstract and unknown to this layer.***/
                tableAdd(metricsTable,nodeIP,emptyEntry);
                metricValue = tableRead(metricsTable,nodeIP);
                decodeMetricValue(metricBuffer,metricValue);
                tableUpdate(metricsTable,nodeIP,metricValue);
            }
            token = strtok(NULL, "|");
        }

    }

}

/**
 * onNetworkEventStrategyInject
 * This function is triggered in response to specific events reported by the network layer
 *
 * @param networkEvent - The network event type that triggered this handler.
 * @param involvedIP - The IP address involved in the network event.
 *
 * @return void
 */
void onNetworkEventStrategyInject(int networkEvent, uint8_t involvedIP[4]){
    switch (networkEvent) {
        case NETEVENT_JOINED_NETWORK:
            encodeMessageStrategyInject(smallSendBuffer, sizeof(smallSendBuffer),INJECT_NODE_INFO);
            sendMessage(involvedIP,smallSendBuffer);
            LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/INJECT_NODE_INFO] message: \"%s\" to: %hhu.%hhu.%hhu.%hhu\n",smallSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);
            break;
        case NETEVENT_CHILD_CONNECTED:
            encodeMessageStrategyInject(largeSendBuffer, sizeof(largeSendBuffer),INJECT_TABLE_INFO);
            sendMessage(involvedIP,largeSendBuffer);
            LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/INJECT_TABLE_INFO] message: \"%s\" to: %hhu.%hhu.%hhu.%hhu\n",largeSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);
            break;
        case NETEVENT_CHILD_DISCONNECTED:
            break;
        default:
            break;
    }
}

/**
 * influenceRoutingStrategyInject
 * Influences the next hop of a data message. When called, this function selects the next hop as the node with the best metric, using the user-provided comparison function.
 * If the best metric node matches the original destination, it sends the message directly.
 * Otherwise, the original message is tunneled inside another data message directed to the best metric node.
 *
 * Example of a encapsulated message:
 * 11 [my IP] [best metric IP] [message payload: 11 [source IP] [original destination IP] [actual payload]]
 *
 * @param dataMessage - The original data message.
 * @return void
 */
void influenceRoutingStrategyInject(char* dataMessage){
    void* bestMetric = nullptr, *currentMetric;
    uint8_t *IP, bestMetricIP[4], *nextHopIP, originalDestination[4];
    bool findBestMetric=false;
    messageParameters params;

    // In the metricsTable, find the node with the best metric using the user-provided comparison function
    for (int i = 0; i < metricsTable->numberOfItems ; i++) {
        IP = (uint8_t *) tableKey(metricsTable,i);
        if(isIPEqual(IP,myIP)) continue;//Skip my IP
        currentMetric = tableRead(metricsTable,IP);
        if(compareMetrics(bestMetric,currentMetric) == 2){
            bestMetric = currentMetric;
            assignIP(bestMetricIP,IP);
            findBestMetric = true;
        }
    }
    LOG(MESSAGES,INFO,"Best Metric Node IP: %hhu.%hhu.%hhu.%hhu\n",bestMetricIP[0],bestMetricIP[1],bestMetricIP[2],bestMetricIP[3]);

    sscanf(dataMessage, "%*d %*u.%*u.%*u.%*u %hhu.%hhu.%hhu.%hhu",&originalDestination[0],&originalDestination[1],&originalDestination[2],&originalDestination[3]);

    // If the best-metric IP address matches the message's original destination, tunneling is not required
    if(isIPEqual(originalDestination,bestMetricIP)){
        //Send the message to the original destination
        nextHopIP = (uint8_t *) findRouteToNode(originalDestination);
        if(nextHopIP != nullptr){
            LOG(MESSAGES,INFO,"Encoded message: %s\n",dataMessage);
            sendMessage(nextHopIP, dataMessage);
            LOG(MESSAGES,INFO,"Sending [DATA] message: \"%s\" to: %hhu.hhu.%hhu.%hhu\n",dataMessage,originalDestination[0],originalDestination[1],originalDestination[2],originalDestination[3]);

        }else{
            LOG(NETWORK,ERROR,"ERROR: Trying to send message to: %hhu.%hhu.%hhu.%hhu but did not find path to node\n",bestMetricIP[0],bestMetricIP[1],bestMetricIP[2],bestMetricIP[3]);
        }
        return;
    }

    // If no best metric is determined, exit without influencing the routing layer
    if(!findBestMetric) return;

    // Encapsulate the original message (generated by the lower layers) inside a new data message,
    // setting its destination to the node with the best routing metric.
    encodeTunneledMessage(largeSendBuffer,sizeof(largeSendBuffer), myIP,bestMetricIP,dataMessage);
    nextHopIP = (uint8_t *) findRouteToNode(bestMetricIP);
    if(nextHopIP != nullptr){
        LOG(MESSAGES,INFO,"Sending tunneled [DATA] message: \"%s\" to: %hhu.%hhu.%hhu.%hhu\n",dataMessage,bestMetricIP[0],bestMetricIP[1],bestMetricIP[2],bestMetricIP[3]);
        sendMessage(nextHopIP, largeSendBuffer);
    }else{
        LOG(NETWORK,ERROR,"ERROR: Trying to send message to: %hhu.%hhu.%hhu.%hhu but did not find path to node\n",bestMetricIP[0],bestMetricIP[1],bestMetricIP[2],bestMetricIP[3]);
    }
}


/**
 * onTimerStrategyInject
 * Periodically sends this node's metric information to all other nodes in the network.
 *
 * @return void
 */
void onTimerStrategyInject(){
    unsigned long currentTime = getCurrentTime();

    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTimeInject) >= MIDDLEWARE_UPDATE_INTERVAL ){
        snprintf(smallSendBuffer, sizeof(smallSendBuffer), "%i ",MIDDLEWARE_MESSAGE);
        encodeMessageStrategyInject(smallSendBuffer, sizeof(smallSendBuffer),INJECT_NODE_INFO);
        LOG(NETWORK,DEBUG,"Sending periodic [MIDDLEWARE/INJECT_NODE_INFO] Message: %s\n",smallSendBuffer);
        propagateMessage(smallSendBuffer,myIP);
        lastMiddlewareUpdateTimeInject = currentTime;
    }
}
/**
 * getContextStrategyInject
 * Retrieves the functions related to the strategy Inject
 *
 * @return void* - pointer to the struct that contains pointers to the context functions
 */
void* getContextStrategyInject(){
    return &injectContext;
}

void setIP(void* av, void* bv){
    uint8_t * a = (uint8_t *) av;
    uint8_t * b = (uint8_t *) bv;
    //Serial.printf("Key.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a[0],a[1],a[2],a[3], b[0],b[1],b[2],b[3]);
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = b[3];
}

/////////// USER Side Functions //////////////////////////
int compareMetrics(void *metricAv,void*metricBv){
    if(metricAv == nullptr && metricBv == nullptr) return -1;

    if(metricAv == nullptr) return 2;

    if(metricBv == nullptr) return 1;

    metricTableEntry *metricA = (metricTableEntry*) metricAv;
    metricTableEntry *metricB = (metricTableEntry*) metricBv;
    if(metricA->processingCapacity >= metricB->processingCapacity){
        return 1;
    }else{
        return 2;
    }
}

void encodeMetricEntry(char* buffer, size_t bufferSize, void *metricEntry){
    metricTableEntry *metric = (metricTableEntry*) metricEntry;
    snprintf(buffer,bufferSize,"%i", metric->processingCapacity);
}

void decodeMetricEntry(char* buffer, void *metricEntry){
    metricTableEntry *metric = (metricTableEntry*)metricEntry;
    sscanf(buffer,"%i", &metric->processingCapacity);
}


void setMetricValue(void* av, void*bv){
    if(bv == nullptr)return; //
    metricTableEntry *a = (metricTableEntry *) av;
    metricTableEntry *b = (metricTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}

void printMetricStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"Node[%hhu.%hhu.%hhu.%hhu] → (Metric: %d) \n",
        ((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2],((uint8_t *)Table->key)[3],
        ((metricTableEntry *)Table->value)->processingCapacity);
}