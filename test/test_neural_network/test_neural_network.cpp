#include <unity.h>
#include <../lib/logger/logger.h>
#include <../lib/neural_network/neural_network.h>
#include "table.h"

//pio test -e native -f "test_neural_network" -v


/*** ****************************** Tests ****************************** ***/

void test_memory_allocation(){
    float weightsValues[3]={1.0,2.0,3.0}, bias=1;
    int inputSize = 3,saveOrderValues[3] ={1,2,3};

    configureNeuron(inputSize, weightsValues,bias,saveOrderValues);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[i]);
        TEST_ASSERT(weights[i] == weightsValues[i]);
        TEST_ASSERT(saveOrder[i] == saveOrderValues[i]);
    }

    delete[] weights;
    delete[] saveOrder;
    delete[] inputs;
}

void test_neuron_output_calculation(){
    for (int i = 0; i < 3; i++) {
        printf("savedWeights:%f\n", weights[i]);
    }
}


void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_memory_allocation);
    RUN_TEST(test_neuron_output_calculation);

    UNITY_END();
}
