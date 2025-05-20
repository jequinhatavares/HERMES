#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "table.h"

//pio test -e native -f "test_middleware_pubsub" -v



/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    int IP[4]={1,1,1,1};
    initMiddlewarePubSub(setPubSubInfo,encodeTopic,decodeTopic);

    tablePrint(pubsubTable, printPubSubStruct);

    tableClean(pubsubTable);

}

void test_add_subscription(){
    int topic = 1, topic2 = 2;

    initMiddlewarePubSub(setPubSubInfo,encodeTopic,decodeTopic);

    //printf("MyIP: %i.%i.%i.%i\n", myIP[0],myIP[1],myIP[2],myIP[3]);
    subscribeToTopic(topic);

    tablePrint(pubsubTable, printPubSubStruct);

    unsubscribeToTopic(topic);

    tablePrint(pubsubTable, printPubSubStruct);

    tableClean(pubsubTable);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);

    tablePrint(pubsubTable, printPubSubStruct);

    unsubscribeToTopic(topic);

    tablePrint(pubsubTable, printPubSubStruct);

    subscribeToTopic(topic2);

    tablePrint(pubsubTable, printPubSubStruct);

}




void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);

    lastModule = NETWORK;
    currentLogLevel = DEBUG;

    int IP[4]={1,1,1,1};
    assignIP(myIP,IP);
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_init_middleware);
    RUN_TEST(test_add_subscription);

    UNITY_END();
}
