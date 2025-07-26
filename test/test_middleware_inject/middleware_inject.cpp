#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/network/src/core/middleware/strategies/strategy_inject/strategy_inject.h"
#include "../lib/network/src/core/table/table.h"

//pio test -e native -f "test_middleware" -v

void setMetricValue(void* av, void*bv);


MetricTableEntry metrics[TABLE_MAX_SIZE];



/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    MetricTableEntry metric;
    uint8_t IP[4]={1,1,1,1};
    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    tablePrint(metricsTable, printMetricsTableHeader,printMetricStruct);

    MetricTableEntry *metricValue = (MetricTableEntry*) tableRead(metricsTable,IP);
    TEST_ASSERT(metricValue != nullptr);

    tableClean(metricsTable);

}

void test_handle_middleware_node_info_message(){
    MetricTableEntry metric;
    uint8_t mynodeIP[4]={2,2,2,2};
    uint8_t nodeIP[4]={1,1,1,1};
    char middlewareMsg[50] = "10 0 1.1.1.1 1.1.1.1 1";

    assignIP(myIP,mynodeIP);

    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    handleMessageStrategyInject(middlewareMsg,sizeof(middlewareMsg));

    tablePrint(metricsTable,printMetricsTableHeader, printMetricStruct);

    MetricTableEntry *metricValue = (MetricTableEntry*) tableRead(metricsTable,nodeIP);
    TEST_ASSERT(metricValue != nullptr);
    TEST_ASSERT(metricValue->processingCapacity == 1);

    tableClean(metricsTable);
}

void test_handle_middleware_table_info_message(){
    MetricTableEntry metric;
    uint8_t node2IP[4]={2,2,2,2};
    uint8_t node3IP[4]={3,3,3,3};
    char middlewareMsg[50] = "10 1 1.1.1.1 |1.1.1.1 1 |2.2.2.2 2 |3.3.3.3 3";

    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    handleMessageStrategyInject(middlewareMsg,sizeof(middlewareMsg));

    tablePrint(metricsTable,printMetricsTableHeader, printMetricStruct);

    MetricTableEntry *metricValue = (MetricTableEntry*) tableRead(metricsTable,myIP);
    TEST_ASSERT(metricValue != nullptr);
    TEST_ASSERT(metricValue->processingCapacity == 1);

    MetricTableEntry *metricValue2 = (MetricTableEntry*) tableRead(metricsTable,node2IP);
    TEST_ASSERT(metricValue2 != nullptr);
    TEST_ASSERT(metricValue2->processingCapacity == 2);

    MetricTableEntry *metricValue3 = (MetricTableEntry*) tableRead(metricsTable,node3IP);
    TEST_ASSERT(metricValue3 != nullptr);
    TEST_ASSERT(metricValue3->processingCapacity == 3);

    tableClean(metricsTable);
}

void test_encode_middleware_node_info_message(){
    MetricTableEntry metric;
    uint8_t nodeIP[4]={2,2,2,2};
    char middlewareMsg[50];
    char correctEncodedMsg[50];
    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 1.1.1.1 1",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);

    snprintf(middlewareMsg, sizeof(middlewareMsg),"%d %d 2.2.2.2 1.1.1.1 1",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);

    encodeMessageStrategyInject(middlewareMsg, sizeof(middlewareMsg),INJECT_NODE_INFO);

    TEST_ASSERT(strcmp(middlewareMsg,correctEncodedMsg) == 0);

    tableClean(metricsTable);
}

void test_encode_middleware_table_info_message(){
    MetricTableEntry metric;
    char middlewareMsg1[50] = "10 0 2.2.2.2 2.2.2.2 2",middlewareMsg2[50] = "10 0 2.2.2.2 3.3.3.3 3", msgBuffer[100];
    char correctEncodedMsg[50] = "10 1 1.1.1.1 |1.1.1.1 1 |2.2.2.2 2 |3.3.3.3 3";
    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    snprintf(middlewareMsg1, sizeof(middlewareMsg1),"%d %d 2.2.2.2 2.2.2.2 2",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);
    snprintf(middlewareMsg2, sizeof(middlewareMsg2),"%d %d 2.2.2.2 3.3.3.3 3",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);

    handleMessageStrategyInject(middlewareMsg1,sizeof(middlewareMsg1));
    handleMessageStrategyInject(middlewareMsg2,sizeof(middlewareMsg2));

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 |1.1.1.1 1 |2.2.2.2 2 |3.3.3.3 3",MIDDLEWARE_MESSAGE,INJECT_TABLE_INFO);

    encodeMessageStrategyInject(largeSendBuffer, sizeof(largeSendBuffer),INJECT_TABLE_INFO);

    printf("Encoded message: %s len:%i\n",largeSendBuffer, strlen(largeSendBuffer));
    printf("Correct message: %s len:%i\n",correctEncodedMsg, strlen(correctEncodedMsg));
    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);/******/

    tableClean(metricsTable);
}

void test_rewrite_sender_with_complex_metric(){
    MetricTableEntry metric;
    char middlewareMsg1[50] = "10 0 2.2.2.2 2.2.2.2 2 3 0,777";
    char correctEncodedMsg[50] = "10 0 1.1.1.1 2.2.2.2 2 3 0,777";
    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    snprintf(middlewareMsg1, sizeof(middlewareMsg1),"%d %d 2.2.2.2 2.2.2.2 2 3 0,777",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);
    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 2.2.2.2 2 3 0,777",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);

    rewriteSenderIPInject(middlewareMsg1, smallSendBuffer,sizeof(smallSendBuffer), INJECT_NODE_INFO);


    printf("Encoded message: %s len:%i\n",middlewareMsg1, strlen(middlewareMsg1));
    printf("Correct message: %s len:%i\n",correctEncodedMsg, strlen(correctEncodedMsg));
    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);/******/

    tableClean(metricsTable);
}

void test_rewrite_sender_with_bigger_ip(){
    MetricTableEntry metric;
    char middlewareMsg1[50] = "10 0 2.2.2.2 2.2.2.2 2 3 0,777";
    char correctEncodedMsg[50] = "10 0 111.111.111.111 2.2.2.2 2 3 0,777";
    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    uint8_t nodeIP[4]={111,111,111,111};
    assignIP(myIP,nodeIP);

    metric.processingCapacity = 1;
    injectNodeMetric(&metric);

    snprintf(middlewareMsg1, sizeof(middlewareMsg1),"%d %d 2.2.2.2 2.2.2.2 2 3 0,777",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);

    rewriteSenderIPInject(middlewareMsg1,smallSendBuffer, sizeof(smallSendBuffer), INJECT_NODE_INFO);

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 111.111.111.111 2.2.2.2 2 3 0,777",MIDDLEWARE_MESSAGE,INJECT_NODE_INFO);

    printf("Encoded message: %s len:%i\n",middlewareMsg1, strlen(smallSendBuffer));
    printf("Correct message: %s len:%i\n",correctEncodedMsg, strlen(correctEncodedMsg));
    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);/******/

    tableClean(metricsTable);
}

void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    uint8_t nodeIP[4]={1,1,1,1};
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
    RUN_TEST(test_rewrite_sender_with_complex_metric);
    RUN_TEST(test_rewrite_sender_with_bigger_ip);
    UNITY_END();
}
