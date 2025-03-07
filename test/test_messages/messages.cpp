#include <unity.h>
#include <cstdio>
#include <string.h>
#include "messages.h"

void test_pdr_correct(){
    char msg[20] = "";
    messageEncode(msg, parentDiscoveryRequest, .IP={1,1,1,1});
    printf(msg);
    TEST_ASSERT(strcmp(msg, "0 1.1.1.1") == 0);
}

void test_pir_incorrect(){
    char msg[20] = "";
    messageEncode(msg, parentInfoResponse, .IP={1,1,1,1});
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
