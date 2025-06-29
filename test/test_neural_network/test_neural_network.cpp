#include <unity.h>
#include <stdio.h>
#include <../lib/logger/logger.h>
#include <../lib/neural_network/core/neuron_core.h>
#include <../lib/neural_network/core/neuron_manager.h>

#define NEURAL_NET_IMPL


#include <../lib/neural_network/coordinator/neural_network_manager.h>
#include "../lib/neural_network/coordinator/nn_parameters.h"



#include "table.h"

//pio test -e native -f "test_neural_network" -v //verbose mode all prints appear
//pio test -e native -f "test_neural_network" // to just see the test results

void printBitField(uint32_t bits, uint8_t size) {
    for (int i = size - 1; i >= 0; --i) {
        printf("%u", (bits >> i) & 1);
    }
    printf("\n");
}


/*** ****************************** Tests ****************************** ***/

void test_memory_allocation(){
    float weightsValues[3]={1.0,2.0,3.0}, bias=1;
    int inputSize = 3,saveOrderValues[3] ={1,2,3},neuronId = 1,neuronStorageIndex = -1;

    configureNeuron(neuronId,inputSize, weightsValues,bias,saveOrderValues);

    neuronStorageIndex = getNeuronStorageIndex(neuronId);
    TEST_ASSERT(neuronStorageIndex != -1);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[neuronStorageIndex][i]);
        TEST_ASSERT(weights[neuronStorageIndex][i] == weightsValues[i]);
        TEST_ASSERT(saveOrders[neuronStorageIndex][i] == saveOrderValues[i]);
    }

    freeAllNeuronMemory();
}

void test_neuron_output_calculation(){
    float weightsValues[3]={1.0,2.0,3.0}, bias=1, neuronOutput, correctNeuronOutput=15;
    int inputSize = 3,saveOrderValues[3] ={1,2,3},neuronId = 1;

    configureNeuron(neuronId, inputSize, weightsValues,bias,saveOrderValues);

    setInput(neuronId,1,1);
    setInput(neuronId,2,2);
    setInput(neuronId,3,3);

    neuronOutput = computeNeuronOutput(neuronId);

    TEST_ASSERT(neuronOutput==correctNeuronOutput);/******/

    freeAllNeuronMemory();

}

void test_handle_message_assign_neuron_one_neuron(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50] ="8 0 |3 2 1 2 2.0 2.0 1";
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2,neuronStorageIndex=-1,neuronId=3;

    handleNeuralNetworkMessage(receivedMessage);

    neuronStorageIndex = getNeuronStorageIndex(neuronId);
    TEST_ASSERT(neuronStorageIndex != -1);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[neuronStorageIndex][i]);
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],saveOrders[neuronStorageIndex][i]);
        TEST_ASSERT(weights[neuronStorageIndex][i] == weightsValues[i]);
        TEST_ASSERT(saveOrders[neuronStorageIndex][i] == saveOrderValues[i]);
    }
    freeAllNeuronMemory();
}


void test_handle_message_assign_neuron_multiple_neurons(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50] ="8 0 |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1";
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2;
    int neuronId1=3,neuronId2=2,neuronStorageIndex1=-1,neuronStorageIndex2=-1;

    handleNeuralNetworkMessage(receivedMessage);

    neuronStorageIndex1 = getNeuronStorageIndex(neuronId1);
    TEST_ASSERT(neuronStorageIndex1 != -1);
    TEST_ASSERT(neuronStorageIndex1 == 0);

    neuronStorageIndex2 = getNeuronStorageIndex(neuronId2);

    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 1);/******/


    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[neuronStorageIndex1][i]);
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],saveOrders[neuronStorageIndex1][i]);
        TEST_ASSERT(weights[neuronStorageIndex1][i] == weightsValues[i]);
        TEST_ASSERT(saveOrders[neuronStorageIndex1][i] == saveOrderValues[i]);

        TEST_ASSERT(weights[neuronStorageIndex2][i] == weightsValues[i]);
        TEST_ASSERT(saveOrders[neuronStorageIndex2][i] == saveOrderValues[i]);/******/
    }

    freeAllNeuronMemory();

}

void test_handle_message_assign_neuron_with_more_than_max_neurons(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[100] ="8 0 |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1|7 2 1 2 2.0 2.0 1";
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2;
    int neuronId1=3,neuronId2=2,neuronId3=7,neuronStorageIndex1=-1,neuronStorageIndex2=-1,neuronStorageIndex3=-1;

    handleNeuralNetworkMessage(receivedMessage);

    neuronStorageIndex1 = getNeuronStorageIndex(neuronId1);
    TEST_ASSERT(neuronStorageIndex1 != -1);
    TEST_ASSERT(neuronStorageIndex1 == 0);

    neuronStorageIndex2 = getNeuronStorageIndex(neuronId2);
    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 1);/******/

    neuronStorageIndex3 = getNeuronStorageIndex(neuronId3);
    TEST_ASSERT(neuronStorageIndex3 == -1);


    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[neuronStorageIndex1][i]);
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],saveOrders[neuronStorageIndex1][i]);
        TEST_ASSERT(weights[neuronStorageIndex1][i] == weightsValues[i]);
        TEST_ASSERT(saveOrders[neuronStorageIndex1][i] == saveOrderValues[i]);

        TEST_ASSERT(weights[neuronStorageIndex2][i] == weightsValues[i]);
        TEST_ASSERT(saveOrders[neuronStorageIndex2][i] == saveOrderValues[i]);/******/
    }

    freeAllNeuronMemory();

}


void test_handle_neuron_input(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50] ="8 0 |3 2 1 2 2.0 2.0 1";
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2, neuronId = 3,outputNeuron1 = 1,outputNeuron2 = 2;
    int neuronStorageIndex = -1;

    //message containing neuron assignments
    handleNeuralNetworkMessage(receivedMessage);

    neuronStorageIndex = getNeuronStorageIndex(neuronId);

    handleNeuronInput(outputNeuron1,1.0);

    handleNeuronInput(outputNeuron2,1.0);

    TEST_ASSERT(neuronStorageIndex  != -1);
    TEST_ASSERT(outputValues[neuronStorageIndex] == 5);

    freeAllNeuronMemory();

}


void test_handle_assign_output_targets(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50],assignNeuronsMessage[50]="8 0 |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1";
    uint8_t nodeIP4[4]={4,4,4,4},nodeIP5[4]={5,5,5,5};
    int neuronStorageIndex = -1;
    uint8_t neuron2=2,neuron3=3;

    //Assign neuron computation to the node
    handleNeuralNetworkMessage(assignNeuronsMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"8 %d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu"
            ,NN_ASSIGN_OUTPUTS,2,neuron2,neuron3,2,nodeIP4[0],nodeIP4[1],nodeIP4[3],nodeIP4[3]
            ,nodeIP5[0],nodeIP5[1],nodeIP5[3],nodeIP5[3]);

    handleNeuralNetworkMessage(receivedMessage);

    TEST_ASSERT(outputTargets[0].nTargets == 2);
    TEST_ASSERT(isIPEqual(outputTargets[0].outputTargets[0],nodeIP4));
    TEST_ASSERT(isIPEqual(outputTargets[0].outputTargets[1],nodeIP5));

    TEST_ASSERT(outputTargets[1].nTargets == 2);
    TEST_ASSERT(isIPEqual(outputTargets[1].outputTargets[0],nodeIP4));
    TEST_ASSERT(isIPEqual(outputTargets[1].outputTargets[1],nodeIP5));
}



void test_encode_message_assign_neuron(){
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char correctMessage[50] ="|3 2 1 2 2 2 1",buffer[200];
    float weightsValues[2]={2.0,2.0}, bias=1;
    uint8_t saveOrderValues[2] ={1,2}, inputSize=2, neuronId = 3;

    encodeAssignNeuronMessage(buffer, sizeof(buffer),neuronId,inputSize,saveOrderValues,weightsValues,bias);

    printf("Encoded Message:%s\n",buffer);
    printf("Encoded Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}

void test_encode_message_neuron_output(){
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50],buffer[200];
    int outputNeuronId = 1;
    float neuronOutput = 2.0;

    snprintf(correctMessage, sizeof(correctMessage),"%d %d %g", NN_NEURON_OUTPUT,outputNeuronId,neuronOutput);

    encodeNeuronOutputMessage(buffer, sizeof(buffer),outputNeuronId, neuronOutput);

    printf("Encoded Message:%s\n",buffer);
    printf("Encoded Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}

void test_encode_NACK_message(){
    //DATA_MESSAGE NN_NACK [Neuron ID with Missing Output] [Missing Output ID 1] [Missing Output ID 2] ...
    char correctMessage[50],buffer[200];
    int outputNeuronId = 1, missingInputs[4]={2,3,4,5};
    float neuronOutput = 2.0;

    snprintf(correctMessage, sizeof(correctMessage),"%d %d %d %d %d", NN_NACK, missingInputs[0],missingInputs[1],missingInputs[2],missingInputs[3]);

    encodeNACKMessage(buffer, sizeof(buffer),missingInputs,4);

    printf("Encoded Message:%s\n",buffer);
    printf("Encoded Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}

void test_encode_ACK_message(){
    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...
    char correctMessage[50],buffer[200];
    int ackInputs[4]={2,3,4,5};

    snprintf(correctMessage, sizeof(correctMessage),"%d %d %d %d %d", NN_NACK, ackInputs[0],ackInputs[1],ackInputs[2],ackInputs[3]);

    encodeNACKMessage(buffer, sizeof(buffer),ackInputs,4);

    printf("Encoded Message:%s\n",buffer);
    printf("Encoded Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}


void test_bit_fields(){
    BitField& bits = receivedInputs[0];

    // 1. Test resetAll
    resetAll(bits);
    TEST_ASSERT(bits == 0);

    // 2. Test setBit
    setBit(bits, 1);
    setBit(bits, 3);
    printBitField(bits, 5);  // Expected: 01010
    TEST_ASSERT((bits & (1U << 1)) != 0);
    TEST_ASSERT((bits & (1U << 3)) != 0);

    // 3. Test countBits
    uint8_t c = countBits(bits);
    TEST_ASSERT(c == 2);

    // 4. Test allBits
    resetAll(bits);
    setBit(bits, 0);
    setBit(bits, 1);
    setBit(bits, 2);
    TEST_ASSERT(allBits(bits, 3));

    // Final output
    printBitField(bits, 5);

    resetAll(bits);
}

/***********************- Test on coordinator node side -*************************/

void test_distribute_neurons(){
    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron2=2,neuron3=3,neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;

    initNeuralNetwork();
    distributeNeuralNetwork(&network, nodes,4);
    tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    NeuronEntry* neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron2);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 0);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron3);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 0);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron4);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 0);
    TEST_ASSERT(neuronEntry->indexInLayer == 2);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron5);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 0);
    TEST_ASSERT(neuronEntry->indexInLayer == 3);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron6);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[2]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron7);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[2]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron8);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[3]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 2);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron9);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[3]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 3);/******/

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron10);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,myIP));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);/******/

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron11);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,myIP));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);/******/

    tableClean(neuronToNodeTable);

}

void test_assign_outputs() {
    char correctMessage[100];
    uint8_t nodes[4][4] = {
            {2, 2, 2, 2},
            {3, 3, 3, 3},
            {4, 4, 4, 4},
            {5, 5, 5, 5},
    };

    uint8_t neuron2 = 2, neuron3 = 3, neuron4 = 4, neuron5 = 5, neuron6 = 6, neuron7 = 7, neuron8 = 8, neuron9 = 9;

    initNeuralNetwork();
    distributeNeuralNetwork(&network, nodes, 4);
    //assignOutputTargetsToNetwork(nodes,4);

    assignOutputTargetsToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[0]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUTS, 2,neuron2,neuron3,
             2,nodes[2][0],nodes[2][1],nodes[2][3],nodes[2][3]
             ,nodes[3][0],nodes[3][1],nodes[3][3],nodes[3][3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);

    assignOutputTargetsToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[1]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",NN_ASSIGN_OUTPUTS,2,neuron4,neuron5
           ,2,nodes[2][0],nodes[2][1],nodes[2][3],nodes[2][3]
            ,nodes[3][0],nodes[3][1],nodes[3][3],nodes[3][3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);

    assignOutputTargetsToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[2]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu",NN_ASSIGN_OUTPUTS,2,neuron6,neuron7
            ,1,myIP[0],myIP[1],myIP[3],myIP[3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);/******/

    assignOutputTargetsToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[3]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUTS,2,neuron8,neuron9
            ,1,myIP[0],myIP[1],myIP[3],myIP[3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);

    tableClean(neuronToNodeTable);
}



void test_assign_pubsub_info() {
    char correctMessage[100];
    uint8_t nodes[4][4] = {
            {2, 2, 2, 2},
            {3, 3, 3, 3},
            {4, 4, 4, 4},
            {5, 5, 5, 5},
    };

    uint8_t neuron2 = 2, neuron3 = 3, neuron4 = 4, neuron5 = 5, neuron6 = 6, neuron7 = 7, neuron8 = 8, neuron9 = 9;

    initNeuralNetwork();
    distributeNeuralNetwork(&network, nodes, 4);
    //assignOutputTargetsToNetwork(nodes,4);

    assignPubSubInfoToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[0]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUTS,2, neuron2,neuron3,0,1);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);

    assignPubSubInfoToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[1]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUTS,2, neuron4,neuron5,0,1);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);

    assignPubSubInfoToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[2]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUTS,2, neuron6,neuron7,1,2);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);

    assignPubSubInfoToNode(largeSendBuffer, sizeof(largeSendBuffer),nodes[3]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUTS,2, neuron8,neuron9,1,2);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctMessage) == 0);/******/

    tableClean(neuronToNodeTable);
}

void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    uint8_t IP[4] = {1,1,1,1};
    assignIP(myIP,IP);
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_memory_allocation);
    RUN_TEST(test_neuron_output_calculation);

    RUN_TEST(test_handle_message_assign_neuron_one_neuron);
    RUN_TEST(test_handle_message_assign_neuron_multiple_neurons);
    RUN_TEST(test_handle_message_assign_neuron_with_more_than_max_neurons);
    RUN_TEST(test_handle_neuron_input);/******/
    RUN_TEST(test_handle_assign_output_targets);

   RUN_TEST(test_encode_message_assign_neuron);
    RUN_TEST(test_encode_message_neuron_output);
    RUN_TEST(test_encode_NACK_message);
    RUN_TEST(test_encode_ACK_message);

    RUN_TEST(test_bit_fields);
    RUN_TEST(test_distribute_neurons);
    RUN_TEST(test_assign_outputs);
    RUN_TEST(test_assign_pubsub_info); /******/
    UNITY_END();
}
