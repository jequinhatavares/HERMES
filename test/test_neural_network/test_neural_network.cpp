#include <unity.h>
#include <../lib/logger/logger.h>
#include <../lib/neural_network/core/neuron_core.h>
#include <../lib/neural_network/core/neuron_manager.h>

#include <../lib/neural_network/coordinator/neural_network_manager.h>

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
    weights = nullptr;
    delete[] saveOrder;
    saveOrder = nullptr;
    delete[] inputs;/******/
    inputs = nullptr;

}

void test_neuron_output_calculation(){
    float weightsValues[3]={1.0,2.0,3.0}, bias=1, neuronOutput, correctNeuronOutput=15;
    int inputSize = 3,saveOrderValues[3] ={1,2,3};

    configureNeuron(inputSize, weightsValues,bias,saveOrderValues);

    setInput(1,1);
    setInput(2,2);
    setInput(3,3);

    neuronOutput = computeNeuronOutput();

    TEST_ASSERT(neuronOutput==correctNeuronOutput);/******/

    delete[] weights;
    weights = nullptr;
    delete[] saveOrder;
    saveOrder = nullptr;
    delete[] inputs;/******/
    inputs = nullptr;
}

void test_handle_message_assign_neuron_one_neuron(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50] ="8 0 |3 2 1 2 2.0 2.0 1";
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2;

    handleNeuralNetworkMessage(receivedMessage);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[i]);
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],saveOrder[i]);
        TEST_ASSERT(weights[i] == weightsValues[i]);
        TEST_ASSERT(saveOrder[i] == saveOrderValues[i]);
    }

}
void test_handle_message_assign_neuron_multiple_neurons(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50] ="8 0 |3 2 1 2 2.0 2.0 1 |3 2 1 2 2.0 2.0 1";
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2;

    handleNeuralNetworkMessage(receivedMessage);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[i]);
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],saveOrder[i]);
        TEST_ASSERT(weights[i] == weightsValues[i]);
        TEST_ASSERT(saveOrder[i] == saveOrderValues[i]);
    }

}

void test_encode_message_assign_neuron(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char correctMessage[50] ="0 3 2 1 2 2 2 1",buffer[200];
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2, neuronId = 3;

    encodeAssignComputationMessage(buffer, sizeof(buffer),neuronId,inputSize,saveOrderValues,weightsValues,bias);

    printf("Encoded Message:%s\n",buffer);
    printf("Encoded Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
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

void tearDown(void){

}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_memory_allocation);
    RUN_TEST(test_neuron_output_calculation);
    RUN_TEST(test_handle_message_assign_neuron_one_neuron);
    RUN_TEST(test_handle_message_assign_neuron_multiple_neurons);
    RUN_TEST(test_encode_message_assign_neuron);
    UNITY_END();
}
