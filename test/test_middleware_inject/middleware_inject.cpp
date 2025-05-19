#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_inject.h"
#include "table.h"

//pio test -e native -f "test_middleware" -v

void setMetricValue(void* av, void*bv);


metricTableEntry metrics[TableMaxSize];



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
    int mynodeIP[4]={2,2,2,2};
    int nodeIP[4]={1,1,1,1};
    char middlewareMsg[50] = "13 1.1.1.1 1.1.1.1 1";

    assignIP(myIP,mynodeIP);

    initMetricTable(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    handleMiddlewareMessage(middlewareMsg,sizeof(middlewareMsg));

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,nodeIP);
    TEST_ASSERT(metricValue != nullptr);
    TEST_ASSERT(metricValue->processingCapacity == 1);

    tableClean(metricTable);

}

void test_encode_middleware_message(){
    metricTableEntry metric;
    int nodeIP[4]={2,2,2,2};
    char middlewareMsg[50] = " 13 1.1.1.1 1.1.1.1 1", msgBuffer[100];
    char correctEncodedMsg[50] = "13 2.2.2.2 1.1.1.1 1";
    initMetricTable(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    assignIP(myIP,nodeIP);
    metric.processingCapacity = 1;
    updateMiddlewareMetric(&metric,nodeIP);

    encodeMiddlewareMessage(middlewareMsg, sizeof(middlewareMsg));


    TEST_ASSERT(strcmp(middlewareMsg,correctEncodedMsg) == 0);/******/

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
