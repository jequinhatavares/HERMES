#include <unity.h>

#include "logger.h"

void test_logger(){
    char msg[6] = "Hello";
    int IP[4] = {1,2,3,4};
    printf("should start printing after this:\n");
    enableModule(MESSAGES);
    LOG(MESSAGES,DEBUG,"String directly: %s\n", "Hello");
    LOG(MESSAGES,DEBUG,"String: %s\n", msg);
    LOG(MESSAGES,DEBUG,"int directly: %i\n", 1);
    LOG(MESSAGES,DEBUG,"Multiple Arguments directly: %s %i.%i.%i.%i\n", msg, 1, 1, 1, 1);
    LOG(MESSAGES,DEBUG,"Multiple Arguments: %s %i.%i.%i.%i\n", msg, IP[0], IP[1], IP[2], IP[3]);
}

void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_logger);
    UNITY_END();
}
