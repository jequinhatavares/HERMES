#include "strategy_inject.h"


bool isIPEqual(void* a, void* b);
void setIP(void* av, void* bv);

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
        .setKey = setIP,
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
    int messageType, senderIP[4],nodeIP[4],metric;
    char tmpBuffer[20], tmpBuffer2[50];
    // If the encoded message already contains metric information, it means this is a propagation of an already encoded message.
    // In this case, only the sender address needs to be updated before further propagation.
    if( sscanf(messageBuffer,"%i %i.%i.%i.%i %i.%i.%i.%i %i",&messageType,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]
            ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &metric) == 10 ){

        snprintf(messageBuffer,bufferSize,"%i %i.%i.%i.%i %i.%i.%i.%i %i",messageType,myIP[0],myIP[1],myIP[2],myIP[3]
                ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], metric);

    }else { // If the message only contains the type, it indicates that this node should encode its own metric information
        encodeLocalMetric(tmpBuffer, sizeof(tmpBuffer));
        snprintf(tmpBuffer2, sizeof(tmpBuffer2),"%i.%i.%i.%i %s",myIP[0],myIP[1],myIP[2],myIP[3],tmpBuffer);
        strcat(messageBuffer,tmpBuffer2);

    }
}

void encodeLocalMetric(char* messageBuffer, size_t bufferSize){
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
    int senderIP[4],nodeIP[4], type;
    void  *emptyEntry = nullptr;


    //MESSAGE_TYPE [sender IP] [nodeIP] metric
    sscanf(messageBuffer,"%i %i.%i.%i.%i %i.%i.%i.%i %s",&type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
           &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);

    // Check if the nodeIP already exists in the middleware metrics table
    void *metricValue = tableRead(metricTable,nodeIP);
    if(metricValue != nullptr){// If the nodeIP is already in the table update the corresponding metric value
        decodeMetricValue(metricBuffer,metricValue);
        updateMiddlewareMetric(metricValue, nodeIP);
    }else{
        /*** If it is a new node, add the nodeIP to the table as a key with a corresponding dummy metric value (nullptr).
        This ensures that when the key is searched later, the function returns a pointer to a user-allocated struct.
        We cannot store the actual metric here because its structure is abstract and unknown to this layer.***/
        tableAdd(metricTable,nodeIP,emptyEntry);
        metricValue = tableRead(metricTable,nodeIP);
        decodeMetricValue(metricBuffer,metricValue);
        updateMiddlewareMetric(metricValue, nodeIP);

    }

    //Print the updated table
    LOG(MESSAGES,INFO,"Updated Middleware Table\n");
    tablePrint(metricTable,printMetricStruct);

    //Encode this node IP as the sender IP and propagate the message
    encodeMiddlewareMessage(messageBuffer, bufferSize);
    propagateMessage(messageBuffer,senderIP);

    //TODO Dependency injection with the message layer

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