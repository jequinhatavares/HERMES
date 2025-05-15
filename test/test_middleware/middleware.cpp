#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_inject.h"
#include "table.h"

//pio test -e native -f "test_middleware" -v

void setMetricValue(void* av, void*bv);

typedef struct metricTableEntry{
    int processingCapacity;
}metricTableEntry;

metricTableEntry metrics[TableMaxSize];

void setMetricValue(void* av, void*bv){
    if(bv == nullptr)return;
    metricTableEntry *a = (metricTableEntry *) av;
    metricTableEntry *b = (metricTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}

void printMetricStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"Node[%d.%d.%d.%d] → (Metric: %d) \n",
        ((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
        ((metricTableEntry *)Table->value)->processingCapacity);
}



/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    metricTableEntry metric;
    int IP[4]={1,1,1,1};
    initMetricTable(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    updateMiddlewareMetric(&metric,IP);

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,IP);
    TEST_ASSERT(metricValue != nullptr);

    tableClean(metricTable);

}

void test_handle_middleware_message(){
    metricTableEntry metric;
    int nodeIP[4]={1,1,1,1};
    char middlewareMsg[50] = "13 1.1.1.1 1";
    initMetricTable(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    handleMiddlewareMessage(middlewareMsg);

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,nodeIP);
    TEST_ASSERT(metricValue != nullptr);/******/

    tableClean(metricTable);

}

void test_encode_middleware_message(){
    metricTableEntry metric;
    int nodeIP[4]={1,1,1,1};
    char middlewareMsg[50] = "1.1.1.1 1", msgBuffer[100];
    initMetricTable(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    assignIP(myIP,nodeIP);
    metric.processingCapacity = 1;
    updateMiddlewareMetric(&metric,nodeIP);

    encodeMiddlewareMessage(msgBuffer, sizeof(msgBuffer));

    printf("Encoded Message: %s\n",msgBuffer);

    TEST_ASSERT(strcmp(middlewareMsg,msgBuffer) == 0);/******/

    tableClean(metricTable);

}



void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_init_middleware);
    RUN_TEST(test_handle_middleware_message);
    RUN_TEST(test_encode_middleware_message);

    UNITY_END();
}
