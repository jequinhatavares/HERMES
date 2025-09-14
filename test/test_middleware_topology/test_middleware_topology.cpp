#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/network/src/core/middleware/strategies/strategy_topology/strategy_topology.h"
#include "../lib/network/src/core/table/table.h"
#include "../lib/network/src/core/wifi_hal/wifi_hal.h"

//pio test -e native -f "test_middleware_topology" -v

/*************************  User Side Definition  **************************/

struct topologyTableEntry{
    int processingCapacity;
};

topologyTableEntry topologyMetrics[TABLE_MAX_SIZE];


uint8_t* chooseParentByProcessingCapacity(uint8_t * targetNodeIP, uint8_t potentialParents[][4], uint8_t nPotentialParents){
    int maxProcessingCapacity = 0;
    int bestParentIndex = -1;
    topologyTableEntry *topologyMetricValue;

    for (int i = 0; i < nPotentialParents; i++) {
        topologyMetricValue = (topologyTableEntry*) getNodeTopologyMetric(potentialParents[i]);
        if(topologyMetricValue != nullptr){
            LOG(MIDDLEWARE,DEBUG,"Potential Parent: %hhu.%hhu.%hhu.%hhu metric:%d\n",potentialParents[i][0],potentialParents[i][1],potentialParents[i][2],potentialParents[i][3],topologyMetricValue->processingCapacity);
            if(topologyMetricValue->processingCapacity >= maxProcessingCapacity){
                bestParentIndex = i;
                maxProcessingCapacity = topologyMetricValue->processingCapacity;
                LOG(MIDDLEWARE,DEBUG,"Parent Selected\n");
            }
        }
    }
    if(bestParentIndex != -1){
        LOG(MIDDLEWARE,DEBUG,"Chosen Parent: %hhu.%hhu.%hhu.%hhu metric:%d\n",potentialParents[bestParentIndex][0],potentialParents[bestParentIndex][1],potentialParents[bestParentIndex][2],potentialParents[bestParentIndex][3]);
        return potentialParents[bestParentIndex];
    }
    else{ return nullptr;}/******/
    return nullptr;
}

void encodeTopologyMetricEntry(char* buffer, size_t bufferSize, void *metricEntry){
    topologyTableEntry *metric = (topologyTableEntry*) metricEntry;
    snprintf(buffer,bufferSize,"%i", metric->processingCapacity);
}

void decodeTopologyMetricEntry(char* buffer, void *metricEntry){
    topologyTableEntry *metric = (topologyTableEntry*)metricEntry;
    sscanf(buffer,"%i", &metric->processingCapacity);
}


void setTopologyMetricValue(void* av, void*bv){
    if(bv == nullptr)return; //
    topologyTableEntry *a = (topologyTableEntry *) av;
    topologyTableEntry *b = (topologyTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}

void printTopologyMetricStruct(TableEntry* Table){
    LOG(MIDDLEWARE,INFO,"Node[%hhu.%hhu.%hhu.%hhu] → (Topology Metric: %d) \n",
        ((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2],((uint8_t *)Table->key)[3],
        ((topologyTableEntry *)Table->value)->processingCapacity);
}


/********************************* Tests *********************************/

void test_encode_parent_list_advertisement_request(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char correctEncodedMsg[100] ;
    ParentInfo possibleParents[3];
    uint8_t nParents=0, IP[4] = {2,2,2,2},parentIP[4] = {3,3,3,3},mySTAIP[4] = {3,3,3,1};

    for (int i = 0; i < 3; i++) {
        assignIP(possibleParents[i].parentIP,IP);
        possibleParents[i].rootHopDistance = 1;
        possibleParents[i].nrOfChildren = 1;
        nParents++;
    }

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 3.3.3.1 1.1.1.1 2.2.2.2 2.2.2.2 2.2.2.2",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT_REQUEST);

    encodeParentListAdvertisementRequest(largeSendBuffer, sizeof(largeSendBuffer), possibleParents, nParents,mySTAIP);

    printf("Encoded message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", largeSendBuffer);

    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);
}

void test_handle_parent_advertisement_request(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char PAR[100] = "11 0 1.1.1.1 1.1.1.0 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5";
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char correctEncodedMsg[100] = "11 1 4.4.4.4 1.1.1.1 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5";

    snprintf(PAR, sizeof(PAR),"%d %d 1.1.1.1 1.1.1.0 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT_REQUEST);
    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 4.4.4.4 1.1.1.1 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT);

    handleMessageStrategyTopology(PAR, sizeof(PAR));

    printf("Correct message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", largeSendBuffer);

    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);
}


void test_init_strategy_topology(){

    iamRoot = true;

    initStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,printTopologyMetricStruct,chooseParentByProcessingCapacity);

}

void test_root_handle_message_metrics_report(){
    //MESSAGE_TYPE TOP_METRICS_REPORT [destination:rootIP] [nodeIP] [metric]
    char MR1[100] = "10 3 1.1.1.1 2.2.2.2 2",MR2[100] = "10 3 1.1.1.1 3.3.3.3 3",MR3[100] = "10 3 1.1.1.1 4.4.4.4 4";
    char correctEncodedMsg[100] = "10 2 3.3.3.3 2.2.2.2 5.5.5.5";
    uint8_t IP1[4]={2,2,2,2}, IP2[4]={3,3,3,3}, IP3[4]={4,4,4,4};

    uint8_t root[4]={1,1,1,1};
    assignIP(rootIP,root);

    iamRoot = true;

    initStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,printTopologyMetricStruct,chooseParentByProcessingCapacity);

    snprintf(MR1, sizeof(MR1),"%d %d 1.1.1.1 2.2.2.2 2",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);
    snprintf(MR2, sizeof(MR2),"%d %d 1.1.1.1 3.3.3.3 3",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);
    snprintf(MR3, sizeof(MR3),"%d %d 1.1.1.1 4.4.4.4 4",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);

    handleMessageStrategyTopology(MR1, sizeof(MR1));
    handleMessageStrategyTopology(MR2, sizeof(MR2));
    handleMessageStrategyTopology(MR3, sizeof(MR3));

    //tablePrint(topologyMetricsTable,printTopologyTableHeader,printTopologyMetricStruct);

    topologyTableEntry *entry1= (topologyTableEntry*) tableRead(topologyMetricsTable,IP1);
    TEST_ASSERT(entry1 != nullptr);
    TEST_ASSERT(entry1->processingCapacity == 2);

    topologyTableEntry *entry2= (topologyTableEntry*) tableRead(topologyMetricsTable,IP2);
    TEST_ASSERT(entry2 != nullptr);
    TEST_ASSERT(entry2->processingCapacity == 3);

    topologyTableEntry *entry3= (topologyTableEntry*) tableRead(topologyMetricsTable,IP3);
    TEST_ASSERT(entry3 != nullptr);
    TEST_ASSERT(entry3->processingCapacity == 4);

    tableClean(topologyMetricsTable);/******/

}

void test_root_handle_parent_advertisement_request(){
    char MR1[100] = "10 3 1.1.1.1 2.2.2.2 2",MR2[100] = "10 3 1.1.1.1 3.3.3.3 3",MR3[100] = "10 3 1.1.1.1 4.4.4.4 4";
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char PAR[100] = "10 0 1.1.1.1 1.1.1.0 5.5.5.5 2.2.2.2 3.3.3.3";
    //MESSAGE_TYPE TOP_PARENT_ASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    char correctEncodedMsg[100] = "10 2 5.5.5.5 5.5.5.5 3.3.3.3";

    iamRoot = true;
    initStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,printTopologyMetricStruct,chooseParentByProcessingCapacity);

    snprintf(MR1, sizeof(MR1),"%d %d 1.1.1.1 2.2.2.2 2",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);
    snprintf(MR2, sizeof(MR2),"%d %d 1.1.1.1 3.3.3.3 3",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);
    snprintf(MR3, sizeof(MR3),"%d %d 1.1.1.1 4.4.4.4 4",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);


    handleMessageStrategyTopology(MR1, sizeof(MR1));
    handleMessageStrategyTopology(MR2, sizeof(MR2));
    handleMessageStrategyTopology(MR3, sizeof(MR3));

    snprintf(PAR, sizeof(PAR),"%d %d 1.1.1.1 1.1.1.0 5.5.5.5 2.2.2.2 3.3.3.3",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT_REQUEST);

    handleMessageStrategyTopology(PAR, sizeof(PAR));

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 5.5.5.5 5.5.5.5 3.3.3.3",MIDDLEWARE_MESSAGE,TOP_PARENT_ASSIGNMENT_COMMAND);

    printf("Correct message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", smallSendBuffer);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);

    tableClean(topologyMetricsTable);

}

void test_root_handle_parent_list_advertisement(){
    //MESSAGE_TYPE TOP_METRICS_REPORT [destination:rootIP] [nodeIP] [metric]
    char MR1[100] = "10 3 1.1.1.1 2.2.2.2 2",MR2[100] = "10 3 1.1.1.1 3.3.3.3 3",MR3[100] = "10 3 1.1.1.1 4.4.4.4 4";
    char PLA[100] = "10 1 1.1.1.1 2.2.2.2 5.5.5.5 2.2.2.2 4.4.4.4";
    char correctEncodedMsg[100];
    uint8_t IP1[4]={2,2,2,2}, IP2[4]={3,3,3,3}, IP3[4]={4,4,4,4};

    uint8_t root[4]={1,1,1,1};
    assignIP(rootIP,root);

    iamRoot = true;

    initStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,printTopologyMetricStruct,chooseParentByProcessingCapacity);

    snprintf(MR1, sizeof(MR1),"%d %d 1.1.1.1 2.2.2.2 2",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);
    snprintf(MR2, sizeof(MR2),"%d %d 1.1.1.1 3.3.3.3 3",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);
    snprintf(MR3, sizeof(MR3),"%d %d 1.1.1.1 4.4.4.4 4",MIDDLEWARE_MESSAGE,TOP_METRICS_REPORT);

    handleMessageStrategyTopology(MR1, sizeof(MR1));
    handleMessageStrategyTopology(MR2, sizeof(MR2));
    handleMessageStrategyTopology(MR3, sizeof(MR3));

    snprintf(PLA, sizeof(PLA),"%d %d 1.1.1.1 2.2.2.2 5.5.5.5 2.2.2.2 4.4.4.4",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT);

    handleMessageStrategyTopology(PLA, sizeof(PLA));

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 2.2.2.2 5.5.5.5 4.4.4.4",MIDDLEWARE_MESSAGE,TOP_PARENT_ASSIGNMENT_COMMAND);

    //printf("Encoded message: %s\n", correctEncodedMsg);
    //printf("Encoded message: %s\n", smallSendBuffer);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);

    tableClean(topologyMetricsTable);
}

void test_root_handle_parent_advertisement_without_any_metric(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char PLA[100] = "10 1 1.1.1.1 2.2.2.2 5.5.5.5 2.2.2.2 4.4.4.4";
    //MESSAGE_TYPE TOP_PARENT_ASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    char correctEncodedMsg[100] = "";

    //Clean the buffer for accurate test
    strcpy(smallSendBuffer,"");
    iamRoot = true;
    initStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,printTopologyMetricStruct,chooseParentByProcessingCapacity);

    snprintf(PLA, sizeof(PLA),"%d %d 1.1.1.1 2.2.2.2 5.5.5.5 2.2.2.2 4.4.4.4",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT);

    handleMessageStrategyTopology(PLA, sizeof(PLA));

    //printf("Encoded message: %s\n", correctEncodedMsg);
    //printf("Encoded message: %s\n", smallSendBuffer);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);

    tableClean(topologyMetricsTable);

}

void test_other_node_handle_parent_list_advertisement(){
    //MESSAGE_TYPE TOP_METRICS_REPORT [destination:rootIP] [nodeIP] [metric]
    char MR1[100] = "10 3 1.1.1.1 2.2.2.2 2",MR2[100] = "10 3 1.1.1.1 3.3.3.3 3",MR3[100] = "10 3 1.1.1.1 4.4.4.4 4";
    char PLA[100] = "10 1 4.4.4.4 2.2.2.2 5.5.5.5 2.2.2.2 4.4.4.4";
    char correctEncodedMsg[100] = "10 2 2.2.2.2 5.5.5.5 4.4.4.4";


    snprintf(PLA, sizeof(PLA),"%d %d 4.4.4.4 2.2.2.2 5.5.5.5 2.2.2.2 4.4.4.4",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT);

    handleMessageStrategyTopology(PLA, sizeof(PLA));

    //printf("Encoded message: %s\n", correctEncodedMsg);
    //printf("Encoded message: %s\n", smallSendBuffer);

    tableClean(topologyMetricsTable);
}

void test_handle_node_update_message(){
    //MESSAGE_TYPE TOP_NODE_UPDATE [nodeIP] [Parent IP] [metric (optional)]
    char NU1[100], NU2[100], NU3[100];
    char correctEncodedMsg[100];
    topologyTableEntry *entry;


    uint8_t root[4]   = {4,4,4,4};
    uint8_t node1[4]  = {2,2,2,2};
    uint8_t node2[4]  = {3,3,3,3};
    uint8_t node3[4]  = {4,4,4,4};
    uint8_t parentIP[4] = {1,1,1,1}; // root as parent

    assignIP(rootIP, root);
    iamRoot = true;

    // init topology
    initStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),
                         setTopologyMetricValue,
                         encodeTopologyMetricEntry,
                         decodeTopologyMetricEntry,
                         printTopologyMetricStruct,
                         chooseParentByProcessingCapacity);

    // Simulate node updates with metrics
    snprintf(NU1, sizeof(NU1), "%d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d",MIDDLEWARE_MESSAGE, TOP_NODE_UPDATE,
             node1[0],node1[1],node1[2],node1[3],parentIP[0],parentIP[1],parentIP[2],parentIP[3],10);
    handleMessageStrategyTopology(NU1, sizeof(NU1));

    entry = (topologyTableEntry*) tableRead(topologyMetricsTable,node1);
    TEST_ASSERT(entry != nullptr);
    TEST_ASSERT(entry->processingCapacity == 10);


    snprintf(NU2, sizeof(NU2), "%d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",
             MIDDLEWARE_MESSAGE, TOP_NODE_UPDATE,node2[0],node2[1],node2[2],node2[3],
             parentIP[0],parentIP[1],parentIP[2],parentIP[3]);
    handleMessageStrategyTopology(NU2, sizeof(NU2));

    entry = (topologyTableEntry*) tableRead(topologyMetricsTable,node2);
    TEST_ASSERT(entry == nullptr);

    // Now encode one back from our own state (simulate node sending update) first without metric just the parent IP
    assignIP(parent, parentIP);

    encodeNodeUpdateMessage(correctEncodedMsg, sizeof(correctEncodedMsg));

    // Verify it matches the expected structure (without metric)
    char expectedMsg[100];
    snprintf(expectedMsg, sizeof(expectedMsg),
             "%d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",MIDDLEWARE_MESSAGE, TOP_NODE_UPDATE,
             myIP[0],myIP[1],myIP[2],myIP[3],parentIP[0],parentIP[1],parentIP[2],parentIP[3]);

    TEST_ASSERT(strcmp(correctEncodedMsg, expectedMsg) == 0);

    // Now encode one back from our own state (simulate node sending update) first with metric
    assignIP(parent, parentIP);

    snprintf(NU3, sizeof(NU3), "%d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d",
             MIDDLEWARE_MESSAGE, TOP_NODE_UPDATE,myIP[0],myIP[1],myIP[2],myIP[3],
             parentIP[0],parentIP[1],parentIP[2],parentIP[3],10);
    handleMessageStrategyTopology(NU3, sizeof(NU3));

    encodeNodeUpdateMessage(correctEncodedMsg, sizeof(correctEncodedMsg));

    // Verify it matches the expected structure (with metric)
    snprintf(expectedMsg, sizeof(expectedMsg),
             "%d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d",MIDDLEWARE_MESSAGE, TOP_NODE_UPDATE,
             myIP[0],myIP[1],myIP[2],myIP[3],parentIP[0],parentIP[1],parentIP[2],parentIP[3],10);

    TEST_ASSERT(strcmp(correctEncodedMsg, expectedMsg) == 0);

    tableClean(topologyMetricsTable);
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

    uint8_t root[4]={4,4,4,4};
    assignIP(rootIP,root);
    iamRoot=false;

}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();/******/
    RUN_TEST(test_init_strategy_topology);
    RUN_TEST(test_encode_parent_list_advertisement_request);

    RUN_TEST(test_handle_parent_advertisement_request);
    RUN_TEST(test_root_handle_message_metrics_report);
    RUN_TEST(test_root_handle_parent_advertisement_request);
    RUN_TEST(test_root_handle_parent_list_advertisement);
    RUN_TEST(test_root_handle_parent_advertisement_without_any_metric);
    RUN_TEST(test_other_node_handle_parent_list_advertisement);/******/
    RUN_TEST(test_handle_node_update_message);
    UNITY_END();
}
