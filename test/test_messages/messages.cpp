#include <unity.h>
#include <cstdio>
#include <string.h>
#include "messages.h"

void test_pdr_correct(){
    char msg[20] = "";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;

    encodeMessage(msg, parentDiscoveryRequest, params);
    printf(msg);
    TEST_ASSERT(strcmp(msg, "0 1.1.1.1") == 0);
}

void test_pir_incorrect(){
    char msg[20] = "";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;

    encodeMessage(msg, parentInfoResponse, params);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "") == 0);
}

void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_pdr_correct);
    //RUN_TEST(test_pir_incorrect);
    UNITY_END();
}
