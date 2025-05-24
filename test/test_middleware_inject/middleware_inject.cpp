#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_inject/strategy_inject.h"
#include "table.h"

//pio test -e native -f "test_middleware" -v

void setMetricValue(void* av, void*bv);


metricTableEntry metrics[TableMaxSize];



/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    metricTableEntry metric;
    int IP[4]={1,1,1,1};
    initMiddlewareInject(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,IP);
    TEST_ASSERT(metricValue != nullptr);

    tableClean(metricTable);

}

void test_handle_middleware_node_info_message(){
    metricTableEntry metric;
    int mynodeIP[4]={2,2,2,2};
    int nodeIP[4]={1,1,1,1};
    char middlewareMsg[50] = "13 0 1.1.1.1 1.1.1.1 1";

    assignIP(myIP,mynodeIP);

    initMiddlewareInject(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    handleMiddlewareMessageInject(middlewareMsg,sizeof(middlewareMsg));

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,nodeIP);
    TEST_ASSERT(metricValue != nullptr);
    TEST_ASSERT(metricValue->processingCapacity == 1);

    tableClean(metricTable);
}

void test_handle_middleware_table_info_message(){
    metricTableEntry metric;
    int node2IP[4]={2,2,2,2};
    int node3IP[4]={3,3,3,3};
    char middlewareMsg[50] = "13 1 1.1.1.1 |1.1.1.1 1 |2.2.2.2 2 |3.3.3.3 3";

    initMiddlewareInject(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    handleMiddlewareMessageInject(middlewareMsg,sizeof(middlewareMsg));

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,myIP);
    TEST_ASSERT(metricValue != nullptr);
    TEST_ASSERT(metricValue->processingCapacity == 1);

    metricTableEntry *metricValue2 = (metricTableEntry*) tableRead(metricTable,node2IP);
    TEST_ASSERT(metricValue2 != nullptr);
    TEST_ASSERT(metricValue2->processingCapacity == 2);

    metricTableEntry *metricValue3 = (metricTableEntry*) tableRead(metricTable,node3IP);
    TEST_ASSERT(metricValue3 != nullptr);
    TEST_ASSERT(metricValue3->processingCapacity == 3);

    tableClean(metricTable);
}

void test_encode_middleware_node_info_message(){
    metricTableEntry metric;
    int nodeIP[4]={2,2,2,2};
    char middlewareMsg[50] = " 13 0 2.2.2.2 1.1.1.1 1", msgBuffer[100];
    char correctEncodedMsg[50] = "13 0 1.1.1.1 1.1.1.1 1";
    initMiddlewareInject(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    encodeMiddlewareMessageInject(middlewareMsg, sizeof(middlewareMsg),INJECT_NODE_INFO);

    TEST_ASSERT(strcmp(middlewareMsg,correctEncodedMsg) == 0);

    tableClean(metricTable);
}

void test_encode_middleware_table_info_message(){
    metricTableEntry metric;
    char middlewareMsg1[50] = " 13 0 2.2.2.2 2.2.2.2 2",middlewareMsg2[50] = " 13 0 2.2.2.2 3.3.3.3 3", msgBuffer[100];
    char correctEncodedMsg[50] = "13 1 1.1.1.1 |1.1.1.1 1 |2.2.2.2 2 |3.3.3.3 3";
    initMiddlewareInject(setMetricValue, (void*) metrics,sizeof(metricTableEntry),encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    handleMiddlewareMessageInject(middlewareMsg1,sizeof(middlewareMsg1));
    handleMiddlewareMessageInject(middlewareMsg2,sizeof(middlewareMsg2));

    encodeMiddlewareMessageInject(largeSendBuffer, sizeof(largeSendBuffer),INJECT_TABLE_INFO);

    //printf("Encoded message: %s len:%i\n",largeSendBuffer, strlen(largeSendBuffer));
    //printf("Correct message: %s len:%i\n",correctEncodedMsg, strlen(correctEncodedMsg));
    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);/******/

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

    int nodeIP[4]={1,1,1,1};
    assignIP(myIP,nodeIP);

}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_init_middleware);
    RUN_TEST(test_handle_middleware_node_info_message);
    RUN_TEST(test_handle_middleware_table_info_message);
    RUN_TEST(test_encode_middleware_node_info_message);
    RUN_TEST(test_encode_middleware_table_info_message);
    UNITY_END();
}
