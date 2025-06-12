#include <unity.h>
#include <cstdio>
#include <stdint.h>
#include <logger.h>


//pio test -e native -f "test_middleware_topology" -v


/*** ****************************** Tests ****************************** ***/

void test_uint8(){
    //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    uint8_t number = 1,sNumber1,sNumber2,eNumber1 = 7 ,eNumber2 = 8;
    char message[30] = "255.255", buffer[10];

    // Test printf of uint8_t
    printf("Printf Test uint8_t number:%hhu\n",number);
    printf("Printf Test uint8_t number:%d\n",number);
    printf("Printf Test uint8_t number:%u\n",number);


    // Test sscanf of uint8_t
    sscanf(message,"%hhu.%hhu",&sNumber1,&sNumber2);
    printf("sNumber1:%d sNumber2:%d\n",sNumber1,sNumber2);
    /*
    sscanf(message,"%d.%d",&sNumber1,&sNumber2);
    printf("sNumber1:%d sNumber2:%d\n",sNumber1,sNumber2);

    sscanf(message,"%u.%u",&sNumber1,&sNumber2);
    printf("sNumber1:%d sNumber2:%d\n",sNumber1,sNumber2);
    */
    //sprintf test of uint8_t

    sprintf(buffer,"%hhu.%hhu",eNumber1,eNumber2);
    printf("Result from sprintf: %s\n", buffer);
    /* */
    /***snprintf(buffer,sizeof(buffer),"%d.%d",eNumber1,eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer,sizeof(buffer),"%u.%u",eNumber1,eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer,sizeof(buffer),"%i.%i",eNumber1,eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer,sizeof(buffer),"%x.%x",eNumber1,eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer,sizeof(buffer),"%d.%d",(int)eNumber1,(int)eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer,sizeof(buffer),"%hhu.%hhu",eNumber1,eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer,sizeof(buffer),"%u.%u",(unsigned int)eNumber1,(unsigned int)eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    sprintf(buffer,"%hhu.%hhu",(unsigned int)eNumber1,(unsigned int)eNumber2);
    printf("Result from sprintf: %s\n", buffer);

    snprintf(buffer, sizeof(buffer), "%" PRIu8, eNumber1);  // Most portable way
    printf("Result from sprintf: %s\n", buffer);***/


    //int a, b;
    //a = (int) eNumber1;
    //b = (int) eNumber2;
    //sprintf(buffer,"%d.%d",a,b);
    //printf("Result from sprintf-1: %s\n", buffer);


    //sprintf(buffer,"%c.%c",eNumber1,eNumber2);
    //printf("Result from sprintf-2: %s\n", buffer);
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
    RUN_TEST(test_uint8);
    //RUN_TEST(test_handle_parent_assignment_command);
    UNITY_END();
}
