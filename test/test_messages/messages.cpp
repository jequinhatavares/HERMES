#include <unity.h>
#include <cstdio>
#include <string.h>
#include "messages.h"

//pio test -e native -f "test_messages" -v

void test_pdr_correct(){
    char msg[20] = "";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;

    encodeMessage(msg,sizeof(msg),PARENT_DISCOVERY_REQUEST, params);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "0 1.1.1.1") == 0);
}

void test_pir_incorrect(){
    char msg[20] = "";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;

    encodeMessage(msg,sizeof(msg), PARENT_INFO_RESPONSE, params);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "") == 0);
}

void test_data_messages_encoding(){
    char msg[20] = "Hello", messageBuffer[100];
    char correctMessage[100]= "11 Hello 1.1.1.1 2.2.2.2";
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;
    params.IP2[0] = 2; params.IP2[1] = 2; params.IP2[2] = 2; params.IP2[3] = 2;

    strcpy(params.payload, msg);

    encodeMessage(messageBuffer,sizeof(messageBuffer),DATA_MESSAGE,params);


    printf("Encoded Message: %s\n", messageBuffer);

    TEST_ASSERT(strcmp(messageBuffer,correctMessage) == 0);
}

void test_data_messages_encoding_with_string(){
    char correctMessage[100]= "10 Hello 1.1.1.1 2.2.2.2", messageBuffer[100];
    messageParameters params;
    params.IP1[0] = 1; params.IP1[1] = 1; params.IP1[2] = 1; params.IP1[3] = 1;
    params.IP2[0] = 2; params.IP2[1] = 2; params.IP2[2] = 2; params.IP2[3] = 2;

    strcpy(params.payload, "Hello");

    encodeMessage(messageBuffer,sizeof(messageBuffer),DATA_MESSAGE,params);


    printf("Encoded Message2: %s, size of buffer:%i\n", messageBuffer, sizeof(messageBuffer));
    printf("StrCmp: %i\n", strcmp(messageBuffer,correctMessage));

    //TEST_ASSERT(strcmp(messageBuffer,correctMessage) == 0);
}


void test_PDR_messages_invalid_encoding(){

    char incorrectMessage[100]= "1 1.1.1.1";
    TEST_ASSERT(isMessageValid(PARENT_DISCOVERY_REQUEST,incorrectMessage) == false);

    char incorrectMessage2[100]= "0 1.1.1";
    TEST_ASSERT(isMessageValid(PARENT_DISCOVERY_REQUEST,incorrectMessage2) == false);

    char incorrectMessage3[100]= "0 H.E.L.L.O";
    TEST_ASSERT(isMessageValid(PARENT_DISCOVERY_REQUEST,incorrectMessage3) == false);

    char incorrectMessage4[100]= "0 9 9 9 9 ";
    TEST_ASSERT(isMessageValid(PARENT_DISCOVERY_REQUEST,incorrectMessage4) == false);

    char correctMessage[100]= "0 1.1.1.1";
    TEST_ASSERT(isMessageValid(PARENT_DISCOVERY_REQUEST,correctMessage) == true);

}

void test_PIR_messages_invalid_encoding(){

    char incorrectMessage[100]= "0 1.1.1.1";
    TEST_ASSERT(isMessageValid(PARENT_INFO_RESPONSE,incorrectMessage) == false);

    char incorrectMessage2[100]= "1 1.1.1";
    TEST_ASSERT(isMessageValid(PARENT_INFO_RESPONSE,incorrectMessage2) == false);

    char incorrectMessage3[100]= "1 H.E.L.L.O 1 1";
    TEST_ASSERT(isMessageValid(PARENT_INFO_RESPONSE,incorrectMessage3) == false);

    char correctMessage[100]= "1 1.1.1.1 1 1";
    TEST_ASSERT(isMessageValid(PARENT_INFO_RESPONSE,correctMessage) == true);

}

void test_FRTU_messages_invalid_encoding(){

    char incorrectMessage[100]= "4 1.1.1.1 2.2.2.2 |1.1.1.1 2 2 |2.2.2.2 2 1 |2.2.2.2 2 1";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,incorrectMessage) == false);

    char incorrectMessage2[100]= "3 1.1.1.1 |1.1.1.1 2 2 |2.2.2.2 2 1 |2.2.2.2 2 1";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,incorrectMessage2) == false);

    char incorrectMessage3[100]= "3 1.1.1.1 2.2.2.2 |1.1.1.1 2 |2.2.2.2 2 1 |2.2.2.2 2 1";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,incorrectMessage3) == false);

    char incorrectMessage4[100]= "3 1.1.1.1 2.2.2.2 |1.1.1.1 2 |2.2.2.2 2 1 |2.2.2.2 2 1";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,incorrectMessage4) == false);

    /***char incorrectMessage5[100]= "3 1.1.1.1 2.2.2.2 |1.1.1.1 2 1 1|2.2.2.2 2 1 |2.2.2.2 2 1";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,incorrectMessage5) == true);***/

    char correctMessage[100]= "3 1.1.1.1 2.2.2.2 |1.1.1.1 2 2 |2.2.2.2 2 1 |2.2.2.2 2 1";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,correctMessage) == true);

}

void test_DATA_messages_invalid_encoding(){

    char incorrectMessage[100]= "12 HELLO 1.1.1.1 2.2.2.2";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,incorrectMessage) == false);

    char incorrectMessage2[100]= "11 1.1.1.1 2.2.2.2 HELLO";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,incorrectMessage2) == false);

    char incorrectMessage3[100]= "11 HELLO 1.1.1.1";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,incorrectMessage3) == false);

    char correctMessage[100]= "11 HELLO 1.1.1.1 2.2.2.2";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,correctMessage) == true);

}

void test_message_validity_with_invalid_IP(){
    char correctMessage[100]= "0 -1.-1.-1.-1";
    TEST_ASSERT(isMessageValid(PARENT_INFO_RESPONSE,correctMessage) == false);
}
void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_pdr_correct);
    //RUN_TEST(test_pir_incorrect);
    RUN_TEST(test_data_messages_encoding);
    RUN_TEST(test_PDR_messages_invalid_encoding);
    RUN_TEST(test_PIR_messages_invalid_encoding);
    RUN_TEST(test_FRTU_messages_invalid_encoding);
    RUN_TEST(test_DATA_messages_invalid_encoding);
    RUN_TEST(test_message_validity_with_invalid_IP);
    UNITY_END();
}
