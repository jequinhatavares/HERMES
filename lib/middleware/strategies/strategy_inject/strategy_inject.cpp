#include "strategy_inject.h"


bool isIPEqual(void* a, void* b);
void setIP(void* av, void* bv);


unsigned long lastMiddlewareUpdateTime = 0;


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
TableInfo* metricTable = &MTable;
int nodes[TableMaxSize][4];

//Function Pointers Initializers
void (*encodeMetricValue)(char*,size_t,void *) = nullptr;
void (*decodeMetricValue)(char*,void *) = nullptr;



void initMetricTable(void (*setValueFunction)(void*,void*), void *metricStruct, size_t metricStructSize,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)) {
    metricTable->setValue = setValueFunction;
    tableInit(metricTable, nodes, metricStruct, sizeof(int[4]), metricStructSize);

    encodeMetricValue = encodeMetricFunction;
    decodeMetricValue = decodeMetricFunction;

}

void updateMiddlewareMetric(void* metricStruct, void* nodeIP){
    int tableIndex = tableFind(metricTable,nodeIP);
    if(tableIndex == -1){ //The node is not yet in the table
        tableAdd(metricTable, nodeIP, metricStruct);
    }else{ //The node is already present in the table
        tableUpdate(metricTable, nodeIP, metricStruct);
    }
}

void injectNodeMetric(void* metric){
   updateMiddlewareMetric(metric, myIP);
}

void encodeMiddlewareMessage(char* messageBuffer, size_t bufferSize){
    int messageType, senderIP[4],nodeIP[4],metric,offset = 0;
    int *entryIP;
    char tmpBuffer[20], tmpBuffer2[50];
    InjectMessageType type;
    void *metricValue;

    // If the encoded message already contains metric information, it means this is a propagation of an already encoded message.
    // In this case, only the sender address needs to be updated before further propagation.
    if( sscanf(messageBuffer,"%i %i.%i.%i.%i %i.%i.%i.%i %i",&messageType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]
            ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &metric) == 10 ){

        snprintf(messageBuffer,bufferSize,"%i %i.%i.%i.%i %i.%i.%i.%i %i",messageType,myIP[0],myIP[1],myIP[2],myIP[3]
                ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], metric);

    }else { // If the message only contains the type, it indicates that this node should encode its own metric information

        if(type == INJECT_NODE_INFO){
            //MESSAGE_TYPE INJECT_TYPE [sender IP] [nodeIP] metric
            encodeMyMetric(tmpBuffer, sizeof(tmpBuffer));
            snprintf(tmpBuffer2, sizeof(tmpBuffer2),"%i %i %i.%i.%i.%i %s",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO,myIP[0],myIP[1],myIP[2],myIP[3],tmpBuffer);
            strcat(messageBuffer,tmpBuffer2);
        }else{
            //MESSAGE_TYPE INJECT_TYPE [sender IP] |[nodeIP] metric |[nodeIP] metric |...
            offset = snprintf(messageBuffer, sizeof(tmpBuffer2),"%i %i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,INJECT_TABLE_INFO,myIP[0],myIP[1],myIP[2],myIP[3]);

            for (int i = 0; i < metricTable->numberOfItems; i++) {
                entryIP = (int*)tableKey(metricTable,i);
                metricValue = tableRead(metricTable,entryIP);
                if(metricValue != nullptr){
                    decodeMetricValue(tmpBuffer,metricValue);
                    offset += snprintf(messageBuffer + offset, bufferSize - offset," |%i.%i.%i.%i %s",entryIP[0],entryIP[1],entryIP[2],entryIP[3],tmpBuffer);
                }
            }
        }
    }
}

void encodeMyMetric(char* messageBuffer, size_t bufferSize){
    char tmpBuffer[20];

    snprintf(messageBuffer,bufferSize,"%i.%i.%i.%i ", myIP[0],myIP[1],myIP[2],myIP[3]);

    void* metricValue = tableRead(metricTable, myIP);
    if(metricValue != nullptr){
        encodeMetricValue(tmpBuffer, sizeof(tmpBuffer),metricValue);
        strcat(messageBuffer, tmpBuffer);
    }
}

void handleMiddlewareMessage(char* messageBuffer, size_t bufferSize){
    char metricBuffer[20];
    int senderIP[4],nodeIP[4];
    void  *emptyEntry = nullptr;
    InjectMessageType injectType;


    //Extract Inject Message Types
    sscanf(messageBuffer,"%*i %i",&injectType);

    if(injectType == INJECT_NODE_INFO){
        //MESSAGE_TYPE INJECT_NODE_INFO [sender IP] [nodeIP] metric
        sscanf(messageBuffer,"%*i %i %i.%i.%i.%i %i.%i.%i.%i %s",&injectType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
               &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);

        // Check if the nodeIP already exists in the middleware metrics table
        void *metricValue = tableRead(metricTable,nodeIP);
        if(metricValue != nullptr){// If the nodeIP is already in the table update the corresponding metric value
            decodeMetricValue(metricBuffer,metricValue);
            tableUpdate(metricTable,nodeIP,metricValue);
        }else{
            /*** If it is a new node, add the nodeIP to the table as a key with a corresponding dummy metric value (nullptr).
            This ensures that when the key is searched later, the function returns a pointer to a user-allocated struct.
            We cannot store the actual metric here because its structure is abstract and unknown to this layer.***/
            tableAdd(metricTable,nodeIP,emptyEntry);
            metricValue = tableRead(metricTable,nodeIP);
            decodeMetricValue(metricBuffer,metricValue);
            tableUpdate(metricTable,nodeIP,metricValue);
        }
        //Print the updated table
        LOG(MESSAGES,INFO,"Updated Middleware Table\n");
        tablePrint(metricTable,printMetricStruct);

        //Encode this node IP as the sender IP and propagate the message
        encodeMiddlewareMessage(messageBuffer, bufferSize);
        propagateMessage(messageBuffer,senderIP);

    }else if(injectType == INJECT_TABLE_INFO){
        //MESSAGE_TYPE INJECT_TABLE_INFO [sender IP] |[nodeIP] metric |[nodeIP] metric |...
        char* token = strtok(messageBuffer, "|");
        //To discard the message type and ensure the token points to the first routing table update entry
        token = strtok(NULL, "|");
        while (token != NULL) {
            sscanf(messageBuffer,"%i.%i.%i.%i %s",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);

            // Check if the nodeIP already exists in the middleware metrics table
            void *metricValue = tableRead(metricTable,nodeIP);
            if(metricValue != nullptr){// If the nodeIP is already in the table update the corresponding metric value
                decodeMetricValue(metricBuffer,metricValue);
                tableUpdate(metricTable,nodeIP,metricValue);
            }else{
                /*** If it is a new node, add the nodeIP to the table as a key with a corresponding dummy metric value (nullptr).
                This ensures that when the key is searched later, the function returns a pointer to a user-allocated struct.
                We cannot store the actual metric here because its structure is abstract and unknown to this layer.***/
                tableAdd(metricTable,nodeIP,emptyEntry);
                metricValue = tableRead(metricTable,nodeIP);
                decodeMetricValue(metricBuffer,metricValue);
                tableUpdate(metricTable,nodeIP,metricValue);
            }
            token = strtok(NULL, "|");
        }

    }

}

void middlewareInfluenceRouting(char* dataMessage){
    void* bestMetric = nullptr, *currentMetric;
    int *IP, bestMetricIP[4], *nextHopIP, originalDestination[4];
    bool findBestMetric=false;
    messageParameters params;

    for (int i = 0; i < metricTable->numberOfItems ; i++) {
        IP = (int*) tableKey(metricTable,i);
        if(isIPEqual(IP,myIP)) continue;//Skip my IP
        currentMetric = tableRead(metricTable,IP);
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

void middlewareOnTimer(){
    unsigned long currentTime = getCurrentTime();
    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTime) >= MIDDLEWARE_UPDATE_INTERVAL ){
        snprintf(smallSendBuffer, sizeof(smallSendBuffer), "%i ",MIDDLEWARE_MESSAGE);
        encodeMiddlewareMessage(smallSendBuffer, sizeof(smallSendBuffer));
        propagateMessage(smallSendBuffer,myIP);
        LOG(NETWORK,DEBUG,"Sending [MIDDLEWARE] Message: %s\n",smallSendBuffer);
        lastMiddlewareUpdateTime = currentTime;
    }
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