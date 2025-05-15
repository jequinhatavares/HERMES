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

int nodeIP[TableMaxSize][4];

//Function Pointers Initializers
void (*encodeMetricValue)(char*,size_t,void *) = nullptr;
void (*decodeMetricValue)(char*,void *) = nullptr;


void initMetricTable(void (*setValueFunction)(void*,void*), void *metricStruct, size_t metricStructSize,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)) {
    metricTable->setValue = setValueFunction;
    tableInit(metricTable, nodeIP, metricStruct, sizeof(int[4]), metricStructSize);

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
    char tmpBuffer[20];

    snprintf(messageBuffer,bufferSize,"%i.%i.%i.%i ", myIP[0],myIP[1],myIP[2],myIP[3]);

    void* metricValue = tableRead(metricTable, myIP);
    if(metricValue != nullptr){
        encodeMetricValue(tmpBuffer, sizeof(tmpBuffer),metricValue);
        strcat(messageBuffer, tmpBuffer);
    }

}

void handleMiddlewareMessage(char* messageBuffer){
    char metricBuffer[20];
    int nodeIP[4], type;
    void  *emptyEntry = nullptr;
    sscanf(messageBuffer,"%i %i.%i.%i.%i %s",&type,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],metricBuffer);

    void *metricValue = tableRead(metricTable,nodeIP);
    if(metricValue != nullptr){
        decodeMetricValue(metricBuffer,metricValue);
        updateMiddlewareMetric(metricValue, nodeIP);
    }else{
        tableAdd(metricTable,nodeIP,emptyEntry);
        metricValue = tableRead(metricTable,nodeIP);
        decodeMetricValue(metricBuffer,metricValue);
        updateMiddlewareMetric(metricValue, nodeIP);
    }

    //TODO propagate message


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



void setIP(void* av, void* bv){
    int* a = (int*) av;
    int* b = (int*) bv;
    //Serial.printf("Key.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a[0],a[1],a[2],a[3], b[0],b[1],b[2],b[3]);
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = b[3];
}

void setMetricValue(void* av, void*bv){
    if(bv == nullptr)return; //
    metricTableEntry *a = (metricTableEntry *) av;
    metricTableEntry *b = (metricTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}