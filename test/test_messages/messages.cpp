#include <unity.h>
#include <cstdio>
#include <string.h>

#include "../lib/network/src/core/routing/messages.h"

//pio test -e native -f "test_messages" -v

void test_pdr_correct(){
    char msg[20] = "";
    uint8_t IP1[4]={1,1,1,1};

    encodeParentDiscoveryRequest(msg,sizeof(msg),IP1);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "0 1.1.1.1") == 0);
}

void test_pir_incorrect(){
    char msg[20] = "";
    uint8_t IP1[4]={1,1,1,1};
    int hopDistance=1, nChildren=1;

    encodeParentInfoResponse(msg,sizeof(msg),IP1,hopDistance,nChildren);
    //printf(msg);
    TEST_ASSERT(strcmp(msg, "1 1.1.1.1 1 1") == 0);
}

void test_data_messages_encoding(){
    char payload[20] = "Hello", messageBuffer[100];
    char correctMessage[100]= "9 1.1.1.1 2.2.2.2 Hello";
    uint8_t senderIP[4]={1,1,1,1},destinationIP[4]={2,2,2,2};

    encodeDataMessage(messageBuffer,sizeof(messageBuffer),payload,senderIP,destinationIP);

    printf("Encoded Message: %s\n", messageBuffer);

    TEST_ASSERT(strcmp(messageBuffer,correctMessage) == 0);
}

void test_data_messages_encoding_with_string(){
    char payload[20] = "Hello Test 2", messageBuffer[50];
    char correctMessage[100]= "9 1.1.1.1 2.2.2.2 Hello Test 2";
    uint8_t senderIP[4]={1,1,1,1},destinationIP[4]={2,2,2,2};

    encodeDataMessage(messageBuffer,sizeof(messageBuffer),payload,senderIP,destinationIP);

    TEST_ASSERT(strcmp(messageBuffer,correctMessage) == 0);
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

    char correctMessage2[100]= "3 135.230.96.1 135.230.96.1 |135.230.96.1 0 4|252.8.107.1 1 2|89.248.169.1 1 2|";
    TEST_ASSERT(isMessageValid(FULL_ROUTING_TABLE_UPDATE,correctMessage2) == true);

}

void test_DATA_messages_invalid_encoding(){

    char incorrectMessage[100]= "9 HELLO 1.1.1.1 2.2.2.2";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,incorrectMessage) == false);

    char incorrectMessage2[100]= "8 HELLO 1.1.1.1 2.2.2.2";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,incorrectMessage2) == false);

    char incorrectMessage3[100]= "8 HELLO 1.1.1.1";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,incorrectMessage3) == false);

    char correctMessage[100]= "9 1.1.1.1 2.2.2.2 HELLO";
    TEST_ASSERT(isMessageValid(DATA_MESSAGE,correctMessage) == true);

}

void test_CRR_messages_validity(){
    char correctMessage[100]= "2 252.8.107.1 135.230.96.100 2";
    TEST_ASSERT(isMessageValid(2,correctMessage) == true);

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
    RUN_TEST(test_data_messages_encoding_with_string);
    RUN_TEST(test_PDR_messages_invalid_encoding);
    RUN_TEST(test_PIR_messages_invalid_encoding);
    RUN_TEST(test_FRTU_messages_invalid_encoding);
    RUN_TEST(test_DATA_messages_invalid_encoding);
    RUN_TEST(test_CRR_messages_validity);
    RUN_TEST(test_message_validity_with_invalid_IP);
    UNITY_END();
}
