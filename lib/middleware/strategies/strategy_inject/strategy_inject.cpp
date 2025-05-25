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
int nodes[TableMaxSize][4];

//Function Pointers Initializers
void (*encodeMetricValue)(char*,size_t,void *) = nullptr;
void (*decodeMetricValue)(char*,void *) = nullptr;



void initStrategyInject(void (*setValueFunction)(void*,void*), void *metricStruct, size_t metricStructSize,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)) {
    metricsTable->setValue = setValueFunction;
    tableInit(metricsTable, nodes, metricStruct, sizeof(int[4]), metricStructSize);

    encodeMetricValue = encodeMetricFunction;
    decodeMetricValue = decodeMetricFunction;

}


void injectNodeMetric(void* metric){
    void*tableEntry = tableRead(metricsTable,myIP);
    if(tableEntry == nullptr){ //The node is not yet in the table
        tableAdd(metricsTable, myIP, metric);
    }else{ //The node is already present in the table
        tableUpdate(metricsTable, myIP, metric);
    }
}

void rewriteSenderIPInject(char* messageBuffer, size_t bufferSize, InjectMessageType type) {
    int messageType, senderIP[4],nodeIP[4],metric;
    InjectMessageType injectType;

    if(type == INJECT_NODE_INFO){
        // If the encoded message already contains metric information, it means this is a propagation of an already encoded message.
        // In this case, only the sender address needs to be updated before further propagation.
        if( sscanf(messageBuffer,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",&messageType,&injectType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]
                ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &metric) == 11 ){

            snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",messageType,injectType,myIP[0],myIP[1],myIP[2],myIP[3]
                    ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], metric);

        }
    }

}
void encodeMessageStrategyInject(char* messageBuffer, size_t bufferSize, int type){
    int messageType, senderIP[4],nodeIP[4],metric,offset = 0;
    int *entryIP;
    char tmpBuffer[20], tmpBuffer2[50];
    void *metricValue;
    InjectMessageType injectType;

    if(type == INJECT_NODE_INFO){
        //MESSAGE_TYPE INJECT_NODE_INFO [sender IP] [nodeIP] metric
        encodeMyMetric(tmpBuffer, sizeof(tmpBuffer));
        snprintf(messageBuffer, bufferSize,"%i %i %i.%i.%i.%i %s",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO,myIP[0],myIP[1],myIP[2],myIP[3],tmpBuffer);
    }else{//INJECT_TABLE_INFO
        //MESSAGE_TYPE INJECT_TABLE_INFO [sender IP] |[nodeIP] metric |[nodeIP] metric |...
        offset = snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,INJECT_TABLE_INFO,myIP[0],myIP[1],myIP[2],myIP[3]);

        for (int i = 0; i < metricsTable->numberOfItems; i++) {
            entryIP = (int*)tableKey(metricsTable,i);
            metricValue = tableRead(metricsTable,entryIP);
            if(metricValue != nullptr){
                encodeMetricValue(tmpBuffer, sizeof(tmpBuffer),metricValue);
                offset += snprintf(messageBuffer + offset, bufferSize - offset," |%i.%i.%i.%i %s",entryIP[0],entryIP[1],entryIP[2],entryIP[3],tmpBuffer);
            }
        }
    }

}

void encodeMyMetric(char* messageBuffer, size_t bufferSize){
    char tmpBuffer[20];

    snprintf(messageBuffer,bufferSize,"%i.%i.%i.%i ", myIP[0],myIP[1],myIP[2],myIP[3]);

    void* metricValue = tableRead(metricsTable, myIP);
    if(metricValue != nullptr){
        encodeMetricValue(tmpBuffer, sizeof(tmpBuffer),metricValue);
        strcat(messageBuffer, tmpBuffer);
    }
}

void handleMessageStrategyInject(char* messageBuffer, size_t bufferSize){
    char metricBuffer[20];
    int senderIP[4],nodeIP[4];
    void  *emptyEntry = nullptr;
    InjectMessageType injectType;

    //Extract Inject Message Types
    sscanf(messageBuffer,"%*i %i",&injectType);

    if(injectType == INJECT_NODE_INFO){
        //MESSAGE_TYPE INJECT_NODE_INFO [sender IP] [nodeIP] metric
        sscanf(messageBuffer,"%*d %d %d.%d.%d.%d %d.%d.%d.%d %s",&injectType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
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
        rewriteSenderIPInject(messageBuffer, bufferSize,INJECT_NODE_INFO);
        propagateMessage(messageBuffer,senderIP);

    }else if(injectType == INJECT_TABLE_INFO){
        //MESSAGE_TYPE INJECT_TABLE_INFO [sender IP] |[nodeIP] metric |[nodeIP] metric |...
        char* token = strtok(messageBuffer, "|");
        //To discard the message type and ensure the token points to the first routing table update entry
        token = strtok(NULL, "|");
        while (token != NULL) {
            sscanf(token,"%d.%d.%d.%d %s",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);
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

void onNetworkEventStrategyInject(int networkEvent, int involvedIP[4]){
    switch (networkEvent) {
        case NETEVENT_JOINED_NETWORK:
            break;
        case NETEVENT_CHILD_CONNECTED:
            encodeMessageStrategyInject(largeSendBuffer, sizeof(largeSendBuffer),INJECT_TABLE_INFO);
            sendMessage(involvedIP,largeSendBuffer);
            break;
        case NETEVENT_CHILD_DISCONNECTED:
            break;
        default:
            break;
    }
}
void influenceRoutingStrategyInject(char* dataMessage){
    void* bestMetric = nullptr, *currentMetric;
    int *IP, bestMetricIP[4], *nextHopIP, originalDestination[4];
    bool findBestMetric=false;
    messageParameters params;

    for (int i = 0; i < metricsTable->numberOfItems ; i++) {
        IP = (int*) tableKey(metricsTable,i);
        if(isIPEqual(IP,myIP)) continue;//Skip my IP
        currentMetric = tableRead(metricsTable,IP);
        if(compareMetrics(bestMetric,currentMetric) == 2){
            bestMetric = currentMetric;
            assignIP(bestMetricIP,IP);
            findBestMetric = true;
        }
    }
    LOG(MESSAGES,INFO,"Best Metric Node IP: %i.%i.%i.%i\n",bestMetricIP[0],bestMetricIP[1],bestMetricIP[2],bestMetricIP[3]);

    sscanf(dataMessage, "%*d %*d.%*d.%*d.%*d %d.%d.%d.%d",&originalDestination[0],&originalDestination[1],&originalDestination[2],&originalDestination[3]);

    // If the best-metric IP address matches the message's original destination, tunneling is not required
    if(isIPEqual(originalDestination,bestMetricIP)) return;

    // If no best metric is determined, exit without influencing the routing layer
    if(!findBestMetric) return;

    // Encapsulate the original message (generated by the lower layers) inside a new data message,
    // setting its destination to the node with the best routing metric.
    encodeTunneledMessage(largeSendBuffer,sizeof(largeSendBuffer), myIP,bestMetricIP,dataMessage);
    nextHopIP = (int*) findRouteToNode(bestMetricIP);
    if(nextHopIP != nullptr){
        LOG(MESSAGES,INFO,"Encoded tunneled message: %s\n",largeSendBuffer);
        sendMessage(nextHopIP, largeSendBuffer);
    }else{
        LOG(NETWORK,ERROR,"ERROR: Trying to send message to: %i.%i.%i.%i but did not find path to node\n",bestMetricIP[0],bestMetricIP[1],bestMetricIP[2],bestMetricIP[3]);
    }
}

void onTimerStrategyInject(){
    unsigned long currentTime = getCurrentTime();
    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTimeInject) >= MIDDLEWARE_UPDATE_INTERVAL ){
        snprintf(smallSendBuffer, sizeof(smallSendBuffer), "%i ",MIDDLEWARE_MESSAGE);
        encodeMessageStrategyInject(smallSendBuffer, sizeof(smallSendBuffer),INJECT_NODE_INFO);
        propagateMessage(smallSendBuffer,myIP);
        LOG(NETWORK,DEBUG,"Sending [MIDDLEWARE] Message: %s\n",smallSendBuffer);
        lastMiddlewareUpdateTimeInject = currentTime;
    }
}

void* getContextStrategyInject(){
    return &injectContext;
}

void setIP(void* av, void* bv){
    int* a = (int*) av;
    int* b = (int*) bv;
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
    LOG(NETWORK,INFO,"Node[%d.%d.%d.%d] → (Metric: %d) \n",
        ((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
        ((metricTableEntry *)Table->value)->processingCapacity);
}