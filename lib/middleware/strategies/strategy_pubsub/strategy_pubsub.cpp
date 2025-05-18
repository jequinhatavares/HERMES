#include "strategy_pubsub.h"


/***
 * Middleware Publish Subscribe table
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



PubSubInfo valuesPubSub[TableMaxSize];

void initMiddleware(void* ){

}

void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize) {

}

void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize) {

}

void middlewareInfluenceRoutingPubSub(char* dataMessage){

}

void setPubSubInfo(void* av, void* bv){
    PubSubInfo *a = (PubSubInfo*) av;
    PubSubInfo *b = (PubSubInfo*) bv;

    for (int i = 0; i < MAX_TOPICS; i++) {
        a->publishedTopics[i] = b->publishedTopics[i];
        a->subscribedTopics[i] = b->subscribedTopics[i];
    }
}
