#include <unity.h>
#include <cstdio>
#include <string.h>
#include "messages.h"

void test_pdr_correct(){
    char msg[20] = "";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;

    encodeMessage(msg, PARENT_DISCOVERY_REQUEST, params);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "0 1.1.1.1") == 0);
}

void test_pir_incorrect(){
    char msg[20] = "";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;

    encodeMessage(msg, PARENT_INFO_RESPONSE, params);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "") == 0);
}

void test_data_messages_encoding(){
    char msg[20] = "Hello", messageBuffer[100];
    char correctMessage[100]= "5 Hello 1.1.1.1 2.2.2.2";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;
    params.IP2[0] = 2; params.IP2[1] = 2; params.IP2[2] = 2; params.IP2[3] = 2;

    strcpy(params.payload, msg);

    encodeMessage(messageBuffer,DATA_MESSAGE,params);


    printf("Encoded Message: %s\n", messageBuffer);

    TEST_ASSERT(strcmp(messageBuffer,correctMessage) == 0);
}

void test_data_messages_encoding_with_string(){
    char correctMessage[100]= "5 Hello 1.1.1.1 2.2.2.2", messageBuffer[100];
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;
    params.IP2[0] = 2; params.IP2[1] = 2; params.IP2[2] = 2; params.IP2[3] = 2;

    strcpy(params.payload, msg);

    encodeMessage(messageBuffer,DATA_MESSAGE,params);


    printf("Encoded Message: %s\n", messageBuffer);

    TEST_ASSERT(strcmp(messageBuffer,correctMessage) == 0);
}
void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_pdr_correct);
    //RUN_TEST(test_pir_incorrect);
    RUN_TEST(test_data_messages_encoding);
    UNITY_END();
}
