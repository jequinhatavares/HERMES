#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_topology/strategy_topology.h"
#include "table.h"
#include "wifi_hal.h"

//pio test -e native -f "test_middleware_topology" -v


/*** ****************************** Tests ****************************** ***/

void test_encode_parent_list_advertisement_request(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char correctEncodedMsg[100] = "13 0 3.3.3.3 3.3.3.1 1.1.1.1 2.2.2.2 2.2.2.2 2.2.2.2";
    parentInfo possibleParents[3];
    int nParents=0, IP[4] = {2,2,2,2},parentIP[4] = {3,3,3,3},mySTAIP[4] = {3,3,3,1};

    for (int i = 0; i < 3; i++) {
        assignIP(possibleParents[i].parentIP,IP);
        possibleParents[i].rootHopDistance = 1;
        possibleParents[i].nrOfChildren = 1;
        nParents++;
    }

    encodeParentListAdvertisementRequest(largeSendBuffer, sizeof(largeSendBuffer), possibleParents, nParents, parentIP, mySTAIP);


    printf("Encoded message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", largeSendBuffer);

    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);
}

void test_handle_parent_advertisement_request(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char PAR[100] = "13 0 1.1.1.1 1.1.1.0 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5";
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char correctEncodedMsg[100] = "13 1 4.4.4.4 1.1.1.1 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5";

    handleMessageStrategyTopology(PAR, sizeof(PAR));

    printf("Encoded message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", largeSendBuffer);

    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);
}

void test_root_handle_parent_advertisement_request(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char PAR[100] = "13 0 1.1.1.1 1.1.1.0 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5";
    //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    char correctEncodedMsg[100] = "13 2 2.2.2.2 2.2.2.2 5.5.5.5";

    iamRoot = true;

    handleMessageStrategyTopology(PAR, sizeof(PAR));

    printf("Encoded message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", smallSendBuffer);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);
}

void test_handle_parent_advertisement(){
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    char PLA[100] = "13 1 1.1.1.1 3.3.3.3 2.2.2.2 5.5.5.5 5.5.5.5 5.5.5.5";
    //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    char correctEncodedMsg[100] = "13 2 3.3.3.3 2.2.2.2 5.5.5.5";

    iamRoot = true;

    handleMessageStrategyTopology(PLA, sizeof(PLA));

    printf("Encoded message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", smallSendBuffer);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);
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

    int root[4]={4,4,4,4};
    assignIP(rootIP,root);

}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_encode_parent_list_advertisement_request);
    RUN_TEST(test_handle_parent_advertisement_request);
    RUN_TEST(test_root_handle_parent_advertisement_request);
    RUN_TEST(test_handle_parent_advertisement);
    UNITY_END();
}
