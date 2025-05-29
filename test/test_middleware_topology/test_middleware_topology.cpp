#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_topology/strategy_topology.h"
#include "table.h"
#include "wifi_hal.h"

//pio test -e native -f "test_middleware_topology" -v


/*** ****************************** Tests ****************************** ***/

void test_encode_inject_parent_list_advertisement(){
    char correctEncodedMsg[100] = "13 0 3.3.3.3 1.1.1.1 2.2.2.1 2.2.2.1 2.2.2.1 2.2.2.1";

    char current_ssid[30] = "JessicaNode2:2:2:2:2:2";

    for (int i = 0; i < 4; i++) {
        strcpy(reachableNetworks.item[reachableNetworks.len], current_ssid);
        reachableNetworks.len++;
    }

    encodeMessageStrategyTopology(largeSendBuffer, sizeof(largeSendBuffer), TOP_PARENT_LIST_ADVERTISEMENT);

    printf("Encoded message: %s\n", correctEncodedMsg);
    printf("Encoded message: %s\n", largeSendBuffer);

    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);
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

    int root[4]={3,3,3,3};
    assignIP(rootIP,root);

}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_encode_inject_parent_list_advertisement);
    UNITY_END();
}
