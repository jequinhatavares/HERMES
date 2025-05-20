#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "table.h"

//pio test -e native -f "test_middleware_pubsub" -v



/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    int IP[4]={1,1,1,1};
    initMiddlewarePubSub(encodeTopic,decodeTopic);

    tablePrint(pubsubTable, printPubSubStruct);

    tableClean(pubsubTable);

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

    UNITY_END();
}
