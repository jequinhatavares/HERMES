#include <unity.h>
#include <stdio.h>
#include <../lib/network/src/core/logger/logger.h>

#include "../lib/neural_network/worker/neuron_core.h"
#include "../lib/neural_network/worker/neuron_manager.h"


#define NEURAL_NET_IMPL

#include "../lib/neural_network/coordinator/neural_network_coordinator.h"


#include "../lib/neural_network/coordinator/nn_parameters.h"
#include "../lib/neural_network/app_globals.h"

#include "../lib/network/src/core/table/table.h"

//pio test -e native -f "test_neural_network_coordinator" -v //verbose mode all prints appear
//pio test -e native -f "test_neural_network_coordinator" // to just see the test results

uint8_t myAPIP[4];
void printBitField(uint32_t bits, uint8_t size) {
    for (int i = size - 1; i >= 0; --i) {
        printf("%u", (bits >> i) & 1);
    }
    printf("\n");
}

//This function mirrors a function implemented but that cannot be tested directly just like this
void assignComputationsToNeuronsWithMissingAcks(uint8_t targetIP[4]){
    NeuronId *currentId;
    NeuronEntry *neuronEntry;
    uint8_t currentLayerIndex, currentIndexInLayer,*inputIndexMap;
    char tmpBuffer[50];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    int i=0,messageOffset=0;

    // Encode the message header for the first node’s neuron assignments
    NeuralNetworkCoordinator::encodeMessageHeader(tmpBuffer, tmpBufferSize,NN_ASSIGN_COMPUTATION);
    messageOffset += snprintf(appPayload, sizeof(appPayload),"%s",tmpBuffer);

    while(i <neuronToNodeTable->numberOfItems){
        currentId = (NeuronId*)tableKey(neuronToNodeTable,i);
        neuronEntry = (NeuronEntry*)tableRead(neuronToNodeTable,currentId);

        /***
         * If the neuron was not acknowledged by the node and it belongs to the current physical device along
         * with other unacknowledged neurons, then include it in the assigning message together with the others.
         ***/
        if(neuronEntry != nullptr && isIPEqual(targetIP,neuronEntry->nodeIP) && !neuronEntry->isAcknowledged && neuronEntry->layer!=0){
            // The current layer index needs to be normalized because the neuronToNode table uses 0 for the input
            // layer, while in the NN structure, index 0 corresponds to the first hidden layer.
            currentLayerIndex = neuronEntry->layer - 1;
            currentIndexInLayer = neuronEntry->indexInLayer;

            //Remake the part of the message that maps the inputs into the input vector
            inputIndexMap = new uint8_t [neuralNetwork.layers[currentLayerIndex].numInputs];
            for (uint8_t j = 0; j < neuralNetwork.layers[currentLayerIndex].numInputs ; j++){
                inputIndexMap[j] = *currentId+(j-neuralNetwork.layers[currentLayerIndex].numInputs);
            }

            //If the neuron is not from the input layer

            //Encode the part assigning neuron information (weights, bias, inputs etc..)
            NeuralNetworkCoordinator::encodeAssignNeuronMessage(tmpBuffer, tmpBufferSize,
                                      *currentId,neuralNetwork.layers[currentLayerIndex].numInputs,inputIndexMap,
                                      &neuralNetwork.layers[currentLayerIndex].weights[currentIndexInLayer * neuralNetwork.layers[currentLayerIndex].numInputs],
                                      neuralNetwork.layers[currentLayerIndex].biases[currentIndexInLayer]);

            messageOffset += snprintf(appPayload + messageOffset, sizeof(appPayload) - messageOffset,"%s",tmpBuffer);

            delete [] inputIndexMap;
        }
            i++;

    }
}


/*** ****************************** Tests ****************************** ***/

void test_memory_allocation(){
    NeuronCore core;
    float weightsValues[3]={1.0,2.0,3.0}, bias=1;
    uint8_t inputSize = 3,saveOrderValues[3] ={1,2,3},neuronId = 1,neuronStorageIndex = -1;

    core.configureNeuron(neuronId,inputSize, weightsValues,bias,saveOrderValues);

    neuronStorageIndex = core.getNeuronStorageIndex(neuronId);
    TEST_ASSERT(neuronStorageIndex != -1);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],core.getNeuronWeightAtIndex(neuronId,i));
        TEST_ASSERT(core.getNeuronWeightAtIndex(neuronId,i) == weightsValues[i]);
        TEST_ASSERT(core.getInputStorageIndex(neuronId,i) == saveOrderValues[i]);
    }

}

void test_neuron_output_calculation(){
    NeuronCore core;

    float weightsValues[3]={1.0,2.0,3.0}, bias=1, neuronOutput, correctNeuronOutput=15;
    uint8_t inputSize = 3,saveOrderValues[3] ={1,2,3},neuronId = 1;

    core.configureNeuron(neuronId, inputSize, weightsValues,bias,saveOrderValues);

    core.setInput(neuronId,1,1);
    core.setInput(neuronId,2,2);
    core.setInput(neuronId,3,3);

    neuronOutput = core.computeNeuronOutput(neuronId);

    TEST_ASSERT(neuronOutput==correctNeuronOutput);/******/

}

void test_handle_message_assign_neuron_one_neuron(){
    NeuronManager neuron;
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50];
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2,neuronStorageIndex=-1,neuronId=3;
    uint8_t blankIP[4]={0,0,0,0};
    snprintf(receivedMessage, sizeof(receivedMessage),"%d |3 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuronStorageIndex = neuron.neuronCore.getNeuronStorageIndex(neuronId);
    TEST_ASSERT(neuronStorageIndex != -1);

    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],neuron.neuronCore.getNeuronWeightAtIndex(neuronId,i));
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],neuron.neuronCore.getInputNeuronId(neuronId,i));
        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId,i) == weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId,i) == saveOrderValues[i]);
    }/******/
}


void test_handle_message_assign_neuron_multiple_neurons(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50];
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2;
    int neuronId1=3,neuronId2=2,neuronStorageIndex1=-1,neuronStorageIndex2=-1;

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuronStorageIndex1 = neuron.neuronCore.getNeuronStorageIndex(neuronId1);
    TEST_ASSERT(neuronStorageIndex1 != -1);
    TEST_ASSERT(neuronStorageIndex1 == 0);

    neuronStorageIndex2 = neuron.neuronCore.getNeuronStorageIndex(neuronId2);

    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 1);/******/


    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],neuron.neuronCore.getNeuronWeightAtIndex(neuronId1,i));
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],neuron.neuronCore.getInputNeuronId(neuronId1,i));
        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId1,i) == weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId1,i) == saveOrderValues[i]);

        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId2,i) == weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId2,i) == saveOrderValues[i]);/******/
    }

}



void test_handle_assign_output_neuron(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //NN_ASSIGN_OUTPUT |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[100];
    char correctMessage[50];
    float weightsValues[4]={2.0,2.0,2.0,2.0}, bias=1;
    int saveOrderValues[4] ={6,7,8,9}, inputSize=4;
    int neuronId10=10,neuronId11=11,neuronStorageIndex10=-1,neuronStorageIndex11=-1;

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |10 4 6 7 8 9 2 2.0 2.0 2.0 2.0 1 |11 4 6 7 8 9 2 2.0 2.0 2.0 2.0 1 ",NN_ASSIGN_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(correctMessage, sizeof(correctMessage),"%d 10 11",NN_ACK);

    neuronStorageIndex10 = neuron.neuronCore.getNeuronStorageIndex(neuronId10);
    TEST_ASSERT(neuronStorageIndex10 != -1);
    TEST_ASSERT(neuronStorageIndex10 == 0);

    neuronStorageIndex11 = neuron.neuronCore.getNeuronStorageIndex(neuronId11);
    TEST_ASSERT(neuronStorageIndex11 != -1);
    TEST_ASSERT(neuronStorageIndex11 == 1);

    neuron.neuronCore.printNeuronInfo();

    for (int i = 0; i < inputSize; i++) {
        //printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],weights[neuronStorageIndex1][i]);
        //printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],saveOrders[neuronStorageIndex1][i]);
        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId10,i) == weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId10,i) == saveOrderValues[i]);

        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId11,i)== weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId11,i) == saveOrderValues[i]);
    }

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);/******/

}

void test_handle_message_assign_neuron_with_more_than_max_neurons(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[100];
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2;
    int neuronId1=3,neuronId2=2,neuronId3=7,neuronStorageIndex1=-1,neuronStorageIndex2=-1,neuronStorageIndex3=-1;

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1|7 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuronStorageIndex1 = neuron.neuronCore.getNeuronStorageIndex(neuronId1);
    TEST_ASSERT(neuronStorageIndex1 != -1);
    TEST_ASSERT(neuronStorageIndex1 == 0);

    neuronStorageIndex2 = neuron.neuronCore.getNeuronStorageIndex(neuronId2);
    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 1);/******/

    neuronStorageIndex3 = neuron.neuronCore.getNeuronStorageIndex(neuronId3);
    TEST_ASSERT(neuronStorageIndex3 == -1);


    for (int i = 0; i < inputSize; i++) {
        printf("weightsValues:%f savedWeights:%f\n",weightsValues[i],neuron.neuronCore.getNeuronWeightAtIndex(neuronId1,i));
        printf("saveOrder:%i saveOrder:%i\n",saveOrderValues[i],neuron.neuronCore.getInputNeuronId(neuronId1,i));
        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId1,i) == weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId1,i) == saveOrderValues[i]);

        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId2,i) == weightsValues[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId2,i) == saveOrderValues[i]);/******/
    }
}


void test_handle_neuron_input(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50], messageWithInput[50];
    float weightsValues[2]={2.0,2.0}, bias=1;
    int saveOrderValues[2] ={1,2}, inputSize=2, neuronId = 3,outputNeuron1 = 1,outputNeuron2 = 2;
    int neuronStorageIndex = -1;

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |3 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    //message containing neuron assignments
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuronStorageIndex = neuron.neuronCore.getNeuronStorageIndex(neuronId);
    TEST_ASSERT(neuronStorageIndex  != -1);

    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(messageWithInput, sizeof(messageWithInput),"%d %i %hhu %g"
            ,NN_NEURON_OUTPUT,0,outputNeuron1,1.0);

    neuron.handleNeuronMessage(blankIP,blankIP,messageWithInput);

    snprintf(messageWithInput, sizeof(messageWithInput),"%d %i %hhu %g"
            ,NN_NEURON_OUTPUT,0,outputNeuron2,1.0);

    neuron.handleNeuronMessage(blankIP,blankIP,messageWithInput);

    printf("Calculated Output:%f\n",neuron.outputValues[neuronStorageIndex]);

    TEST_ASSERT(neuron.outputValues[neuronStorageIndex] == 5);


    neuron.clearAllNeuronMemory();

}


void test_handle_assign_output_targets(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50],assignNeuronsMessage[50];
    uint8_t nodeIP4[4]={4,4,4,4},nodeIP5[4]={5,5,5,5};
    int neuronStorageIndex = -1;
    uint8_t neuron2=2,neuron3=3;

    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    //Assign neuron computation to the node
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu"
            ,NN_ASSIGN_OUTPUT_TARGETS,2,neuron2,neuron3,2,nodeIP4[0],nodeIP4[1],nodeIP4[3],nodeIP4[3]
            ,nodeIP5[0],nodeIP5[1],nodeIP5[3],nodeIP5[3]);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    TEST_ASSERT(neuron.neuronTargets[0].nTargets == 2);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[0],nodeIP4));
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[1],nodeIP5));

    TEST_ASSERT(neuron.neuronTargets[1].nTargets == 2);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[1].targetsIPs[0],nodeIP4));
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[1].targetsIPs[1],nodeIP5));

    neuron.clearAllNeuronMemory();
}


void test_handle_assign_output_targets_to_not_handled_neuron(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};

    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50],assignNeuronsMessage[50],correctACKMessage[50];
    uint8_t nodeIP4[4]={4,4,4,4},nodeIP5[4]={5,5,5,5};
    int neuronStorageIndex = -1;
    uint8_t neuron2=2,invalidNeuron=4;

    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    //Assign neuron computation to the node
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu"
            ,NN_ASSIGN_OUTPUT_TARGETS,2,invalidNeuron,neuron2,2,nodeIP4[0],nodeIP4[1],nodeIP4[3],nodeIP4[3]
            ,nodeIP5[0],nodeIP5[1],nodeIP5[3],nodeIP5[3]);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    for (int i = 0; i < MAX_NEURONS; ++i) {
        printf("Output Target:%i nTargets:%hhu\n",i,neuron.neuronTargets[i].nTargets);
        for (int j = 0; j <neuron.neuronTargets[i].nTargets ; ++j) {
            printf("Output target IP:%hhu.%hhu.%hhu.%hhu\n",neuron.neuronTargets[i].targetsIPs[j][0],
                   neuron.neuronTargets[i].targetsIPs[j][1],neuron.neuronTargets[i].targetsIPs[j][2],neuron.neuronTargets[i].targetsIPs[j][3]);
        }
    }

    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...
    snprintf(correctACKMessage, sizeof(correctACKMessage),"%d %hhu"
            ,NN_ACK,neuron2);


    TEST_ASSERT(neuron.neuronTargets[0].nTargets == 2);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[0],nodeIP4));
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[1],nodeIP5));

    TEST_ASSERT(neuron.neuronTargets[1].nTargets == 0);
    TEST_ASSERT(!isIPEqual(neuron.neuronTargets[1].targetsIPs[0],nodeIP4));
    TEST_ASSERT(!isIPEqual(neuron.neuronTargets[1].targetsIPs[1],nodeIP5));

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctACKMessage);
    TEST_ASSERT(strcmp(appPayload,correctACKMessage) == 0);


    neuron.clearAllNeuronMemory();

}

void test_handle_assign_output_targets_multiple_layer_neurons(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char receivedMessage[50],assignNeuronsMessage[50],correctACKMessage[50];
    uint8_t nodeIP4[4]={4,4,4,4},nodeIP5[4]={5,5,5,5};
    int neuronStorageIndex = -1;
    uint8_t neuron2=2,neuron3=3,invalidNeuron=4;

    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 2 3 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    //Assign neuron computation to the node
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    //|[N Neurons] [neuron ID1] [neuron ID2] ...[N Nodes] [IP Address 1] [IP Address 2] ...
    snprintf(receivedMessage, sizeof(receivedMessage),"%d |%hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu |%hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu"
            ,NN_ASSIGN_OUTPUT_TARGETS,1,neuron2,1,nodeIP4[0],nodeIP4[1],nodeIP4[3],nodeIP4[3]
            ,1,neuron3,1,nodeIP5[0],nodeIP5[1],nodeIP5[3],nodeIP5[3]);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    for (int i = 0; i < MAX_NEURONS; ++i) {
        printf("Output Target:%i nTargets:%hhu\n",i,neuron.neuronTargets[i].nTargets);
        for (int j = 0; j <neuron.neuronTargets[i].nTargets ; ++j) {
            printf("Output target IP:%hhu.%hhu.%hhu.%hhu\n",neuron.neuronTargets[i].targetsIPs[j][0],
                   neuron.neuronTargets[i].targetsIPs[j][1],neuron.neuronTargets[i].targetsIPs[j][2],neuron.neuronTargets[i].targetsIPs[j][3]);
        }
    }

    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...
    snprintf(correctACKMessage, sizeof(correctACKMessage),"%d %hhu %hhu"
            ,NN_ACK,neuron2,neuron3);


    TEST_ASSERT(neuron.neuronTargets[0].nTargets == 1);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[0],nodeIP4));

    TEST_ASSERT(neuron.neuronTargets[1].nTargets == 1);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[1].targetsIPs[0],nodeIP5));

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctACKMessage);
    TEST_ASSERT(strcmp(appPayload,correctACKMessage) == 0);

    neuron.clearAllNeuronMemory();

}

void test_handle_assign_pubsub_info(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;

    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |3 2 1 2 2.0 2.0 1 |2 2 1 2 2.0 2.0 1",NN_ASSIGN_COMPUTATION);


    //Assign neuron computation to the node
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"8 %d |%hhu %hhu %hhu %i %i",NN_ASSIGN_OUTPUT_TARGETS,2,neuron2,neuron3,subTopic,pubTopic);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuron.clearAllNeuronMemory();

}

void test_handle_NACK_without_computed_output(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50]="";
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 1.0;

    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);

    //Assign neuron computation to the node
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d %hhu",NN_NACK,neuron2);

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);
}

void test_handle_NACK_with_computed_output(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;

    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node

    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 0 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 1 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d %hhu",NN_NACK,neuron2);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(correctMessage, sizeof(correctMessage),"%d %i %hhu %g",NN_NEURON_OUTPUT,0,neuron2,outputValue);

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);/******/
}


void test_handle_NACK_for_input_node(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3,inputNeuron=0;
    float outputValue = 5.0;

    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node

    //NN_ASSIGN_INPUT [neuronID]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d 0",NN_ASSIGN_INPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    //NN_FORWARD [neuronId]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0",NN_FORWARD);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d %hhu",NN_NACK,inputNeuron);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(correctMessage, sizeof(correctMessage),"%d %i %hhu %g",NN_NEURON_OUTPUT,0,inputNeuron,5.0);

     printf("Encoded Message:%s\n",appPayload);
     printf("Correct Message:%s\n",correctMessage);
     TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);/******/

}

void test_handle_assign_input_neuron_and_worker_neurons_and_assign_all_outputs(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    uint8_t nodes[4][4] = {
            {2, 2, 2, 2},
            {3, 3, 3, 3},
            {4, 4, 4, 4},
            {5, 5, 5, 5},
    };

    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;

    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node

    //NN_ASSIGN_INPUT [neuronID]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d 0",NN_ASSIGN_INPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    TEST_ASSERT(neuron.inputNeurons[0]==0);

    //receiving the message assigning the outputs
    //|[N Neurons] [neuron ID1] [neuron ID2] ...[N Nodes] [IP Address 1] [IP Address 2] ...
    snprintf(receivedMessage, sizeof(receivedMessage),"%d |2 2 3 2 2.2.2.2 3.3.3.3 |1 0 2 0.0.0.0 4.4.4.4",NN_ASSIGN_OUTPUT_TARGETS);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    TEST_ASSERT(neuron.neuronTargets[0].nTargets == 2);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[0],nodes[0]));
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[0].targetsIPs[1],nodes[1]));

    TEST_ASSERT(neuron.neuronTargets[1].nTargets == 2);
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[1].targetsIPs[0],nodes[0]));
    TEST_ASSERT(isIPEqual(neuron.neuronTargets[1].targetsIPs[1],nodes[1]));

    TEST_ASSERT(neuron.inputTargets.nTargets == 2);
    TEST_ASSERT(isIPEqual(neuron.inputTargets.targetsIPs[0],myAPIP));
    TEST_ASSERT(isIPEqual(neuron.inputTargets.targetsIPs[1],nodes[2]));
/***
    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 0 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 1 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d %hhu",NN_NACK,neuron2);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(correctMessage, sizeof(correctMessage),"%d %i %hhu %g",NN_NEURON_OUTPUT,0,neuron2,outputValue);

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);***/
    neuron.clearAllNeuronMemory();
}

void test_worker_neurons_from_multiple_layers_assigned(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    uint8_t neuron2=2,neuron3=3;
    float weightsValues2[2]={2.0,2.0}, bias2=1;
    int saveOrderValues2[2] ={0,1}, inputSize2=2;

    float weightsValues6[4]={1.0,2.0,1.0,2.0}, bias6=1;
    int saveOrderValues6[4] ={2,3,4,5}, inputSize6=4;
    int neuronId2=2,neuronId6=6,neuronStorageIndex1=-1,neuronStorageIndex2=-1;


    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |6 4 2 3 4 5 1.0 2.0 1.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node

    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuron.neuronCore.printNeuronInfo();

    neuronStorageIndex1 =  neuron.neuronCore.getNeuronStorageIndex(neuronId2);
    TEST_ASSERT(neuronStorageIndex1 != -1);
    TEST_ASSERT(neuronStorageIndex1 == 0);

    neuronStorageIndex2 =  neuron.neuronCore.getNeuronStorageIndex(neuronId6);

    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 1);/******/


    for (int i = 0; i < inputSize2; i++) {
        //printf("weightsValues:%f savedWeights:%f\n",weightsValues2[i],weights[neuronStorageIndex1][i]);
        //printf("saveOrder:%i saveOrder:%i\n",saveOrderValues2[i],saveOrders[neuronStorageIndex1][i]);
        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId2,i) == weightsValues2[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId2,i) == saveOrderValues2[i]);
    }

    for (int i = 0; i < inputSize6; i++) {
        //printf("weightsValues:%f savedWeights:%f\n",weightsValues6[i],weights[neuronStorageIndex2][i]);
        //printf("saveOrder:%i saveOrder:%i\n",saveOrderValues6[i],saveOrders[neuronStorageIndex2][i]);
        TEST_ASSERT(neuron.neuronCore.getNeuronWeightAtIndex(neuronId6,i) == weightsValues6[i]);
        TEST_ASSERT(neuron.neuronCore.getInputNeuronId(neuronId6,i) == saveOrderValues6[i]);

    }

    neuron.clearAllNeuronMemory();

}

void test_worker_compute_neuron_output_having_other_neurons_depending_on_that_output(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    NeuronId neuronId2=2,neuronId6=6;
    float outputValue = 5.0;
    int neuronStorageIndex2,neuronStorageIndex6;
    int inputStorageIndex6;

    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |6 4 2 3 4 5 1.0 2.0 1.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node

    neuronStorageIndex2 = neuron.neuronCore.getNeuronStorageIndex(neuronId2);
    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 0);

    neuronStorageIndex6 = neuron.neuronCore.getNeuronStorageIndex(neuronId6);
    TEST_ASSERT(neuronStorageIndex6 != -1);
    TEST_ASSERT(neuronStorageIndex6 == 1);

    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 0 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 1 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    //printNeuronInfo();

    TEST_ASSERT(neuron.isOutputComputed[neuronStorageIndex2]);

    inputStorageIndex6= neuron.neuronCore.getInputStorageIndex(neuronId6,neuronId2);
    TEST_ASSERT(inputStorageIndex6 != -1);
    TEST_ASSERT(inputStorageIndex6 == 0);
    TEST_ASSERT(isBitSet(neuron.receivedInputs[neuronStorageIndex6],inputStorageIndex6));

    /***snprintf(receivedMessage, sizeof(receivedMessage),"%d %hhu",NN_NACK,neuron2);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(correctMessage, sizeof(correctMessage),"%d %i %hhu %g",NN_NEURON_OUTPUT,0,neuron2,outputValue);

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);/******/
    neuron.clearAllNeuronMemory();

}

void test_worker_produce_input_and_other_neurons_depending_on_that_output(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    NeuronId neuronId2=2,neuronId6=6;
    float outputValue = 5.0;
    int neuronStorageIndex2,neuronStorageIndex6;
    int inputStorageIndex6;

    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node


    neuronStorageIndex2 = neuron.neuronCore.getNeuronStorageIndex(neuronId2);
    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 0);

    neuronStorageIndex6 = neuron.neuronCore.getNeuronStorageIndex(neuronId6);
    TEST_ASSERT(neuronStorageIndex6 != -1);
    TEST_ASSERT(neuronStorageIndex6 == 1);

    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 0 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 1 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    //printNeuronInfo();

    TEST_ASSERT(neuron.isOutputComputed[neuronStorageIndex2]);

    inputStorageIndex6= neuron.neuronCore.getInputStorageIndex(neuronId6,neuronId2);
    TEST_ASSERT(inputStorageIndex6 != -1);
    TEST_ASSERT(inputStorageIndex6 == 0);
    TEST_ASSERT(isBitSet(neuron.receivedInputs[neuronStorageIndex6],inputStorageIndex6));

}


void test_worker_input_node_producing_output_needed_by_other_worker(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],assignNeuronsMessage[50];
    uint8_t nodes[4][4] = {
            {2, 2, 2, 2},
            {3, 3, 3, 3},
            {4, 4, 4, 4},
            {5, 5, 5, 5},
    };

    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;
    int inputStorageIndex2,inputStorageIndex3;
    int neuronStorageIndex2,neuronStorageIndex3;

    //NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);//Assign neuron computation to the node

    //NN_ASSIGN_INPUT [neuronID]
    snprintf(assignNeuronsMessage, sizeof(assignNeuronsMessage),"%d 0",NN_ASSIGN_INPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,assignNeuronsMessage);

    TEST_ASSERT(neuron.inputNeurons[0]==0);

    //NN_FORWARD [neuronId]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0",NN_FORWARD);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    //printNeuronInfo();

    TEST_ASSERT(neuron.inputNeuronsValues[0]==5.0);

    //TEST_ASSERT(isOutputComputed[neuronStorageIndex2]);

    neuronStorageIndex2= neuron.neuronCore.getNeuronStorageIndex(neuron2);
    TEST_ASSERT(neuronStorageIndex2 != -1);
    TEST_ASSERT(neuronStorageIndex2 == 0);
    inputStorageIndex2 = neuron.neuronCore.getInputStorageIndex(neuron2,0);
    TEST_ASSERT(inputStorageIndex2 != -1);
    TEST_ASSERT(isBitSet(neuron.receivedInputs[neuronStorageIndex2],inputStorageIndex2));

    neuronStorageIndex3= neuron.neuronCore.getNeuronStorageIndex(neuron3);
    TEST_ASSERT(neuronStorageIndex3 != -1);
    TEST_ASSERT(neuronStorageIndex3 == 1);
    inputStorageIndex3= neuron.neuronCore.getInputStorageIndex(neuron3,0);
    //printBitField(receivedInputs[inputStorageIndex3],2);
    TEST_ASSERT(isBitSet(neuron.receivedInputs[neuronStorageIndex3],inputStorageIndex3));
/******/
    neuron.clearAllNeuronMemory();

}




void test_encode_message_assign_neuron(){

    //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    char correctMessage[50] ="|3 2 1 2 2 2 1",buffer[200];
    float weightsValues[2]={2.0,2.0}, bias=1;
    uint8_t saveOrderValues[2] ={1,2}, inputSize=2, neuronId = 3;

    NeuralNetworkCoordinator::encodeAssignNeuronMessage(buffer, sizeof(buffer),neuronId,inputSize,saveOrderValues,weightsValues,bias);


    printf("Encoded Message:%s\n",buffer);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}

void test_encode_message_neuron_output(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50],buffer[200];
    int outputNeuronId = 1;
    float neuronOutput = 2.0;

    snprintf(correctMessage, sizeof(correctMessage),"%d %i %d %g",NN_NEURON_OUTPUT,0,outputNeuronId,neuronOutput);

    neuron.encodeNeuronOutputMessage(buffer, sizeof(buffer),0,outputNeuronId, neuronOutput);

    printf("Encoded Message:%s\n",buffer);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}

void test_encode_assign_input_message(){
    //NN_ASSIGN_INPUT [neuronID]
    char correctMessage[50],buffer[200];
    int inputNeuronId = 0;

    snprintf(correctMessage, sizeof(correctMessage),"%d %hhu",NN_ASSIGN_INPUT,inputNeuronId);

    NeuralNetworkCoordinator::encodeInputAssignMessage(buffer, sizeof(buffer),inputNeuronId);

    printf("Encoded Message:%s\n",buffer);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}
void test_encode_NACK_message(){
    //DATA_MESSAGE NN_NACK [Neuron ID with Missing Output] [Missing Output ID 1] [Missing Output ID 2] ...
    char correctMessage[50],buffer[200];
    int outputNeuronId = 1, missingInput= 2;
    float neuronOutput = 2.0;

    snprintf(correctMessage, sizeof(correctMessage)," %d", missingInput);

    NeuronManager::encodeNACKMessage(buffer, sizeof(buffer),missingInput);

    printf("Encoded Message:%s\n",buffer);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}

void test_encode_ACK_message(){
    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...
    char correctMessage[50],buffer[200];
    NeuronId ackInputs[4]={2,3,4,5};

    snprintf(correctMessage, sizeof(correctMessage)," %d %d %d %d", ackInputs[0],ackInputs[1],ackInputs[2],ackInputs[3]);

    NeuronManager::encodeACKMessage(buffer, sizeof(buffer),ackInputs,4);

    printf("Encoded Message:%s\n",buffer);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,buffer) == 0);
}


void test_bit_fields(){
    NeuronManager neuron;
    uint8_t blankIP[4]={0,0,0,0};

    BitField& bits = neuron.receivedInputs[0];

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
    NeuralNetworkCoordinator coordinator;
    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron2=2,neuron3=3,neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;

    //initNeuralNetwork();
    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    //printNeuronInfo();

    tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    NeuronEntry* neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron2);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron3);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron4);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 2);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron5);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 3);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron6);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[2]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron7);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[2]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron8);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[3]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 2);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron9);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[3]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 3);/******/

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron10);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,myAPIP));
    TEST_ASSERT(neuronEntry->layer == 3);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);/******/

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron11);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,myAPIP));
    TEST_ASSERT(neuronEntry->layer == 3);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);/******/

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();

}


void test_distribute_neurons_with_balanced_algorithm(){
    NeuralNetworkCoordinator coordinator;

    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };
    uint8_t neuronsPerNode[4] = {2,2,2,2};

    uint8_t neuron2=2,neuron3=3,neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;

    //initNeuralNetwork();
    coordinator.distributeNeuralNetworkBalanced(&neuralNetwork, nodes,4,neuronsPerNode);
    coordinator.distributeOutputNeurons(&neuralNetwork,myAPIP);

    //printNeuronInfo();

    tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    NeuronEntry* neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron2);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron3);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron4);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 2);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron5);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 1);
    TEST_ASSERT(neuronEntry->indexInLayer == 3);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron6);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[2]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron7);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[2]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron8);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[3]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 2);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron9);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[3]));
    TEST_ASSERT(neuronEntry->layer == 2);
    TEST_ASSERT(neuronEntry->indexInLayer == 3);/******/

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron10);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,myAPIP));
    TEST_ASSERT(neuronEntry->layer == 3);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron11);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,myAPIP));
    TEST_ASSERT(neuronEntry->layer == 3);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);/******/

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();

}

void test_distribute_neural_network_to_one_device(){
    NeuronManager neuron;
    //NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    uint8_t nodes[4][4] = {
            {2,2,2,2},
    };
    uint8_t neuron2=2,neuron3=3,neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;
    char receivedMessage[100];

    //initNeuralNetwork();
    //neuron.distributeNeuralNetwork(&neuralNetwork, nodes,1);

    //printNeuronInfo();

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |2 2 0 1 2.0 2.0 1 |3 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |4 2 0 1 2.0 2.0 1 |5 2 0 1 2.0 2.0 1",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |6 4 2 3 4 5 2.0 2.0 2.0 2.0 1 |7 4 2 3 4 5 2.0 2.0 2.0 2.0 1 ",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    snprintf(receivedMessage, sizeof(receivedMessage),"%d |8 4 2 3 4 5 2.0 2.0 2.0 2.0 1 |9 4 2 3 4 5 2.0 2.0 2.0 2.0 1 ",NN_ASSIGN_COMPUTATION);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    neuron.neuronCore.printNeuronInfo();

    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 0 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    snprintf(receivedMessage, sizeof(receivedMessage),"%d 0 1 1.0",NN_NEURON_OUTPUT);
    neuron.handleNeuronMessage(blankIP,blankIP,receivedMessage);

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();

}

void test_assign_input_neurons(){
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron2=2,neuron3=3,neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;
    uint8_t inputNeuron0 = 0,inputNeuron1 = 1;
    //initNeuralNetwork();
    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    coordinator.distributeInputNeurons( nodes,4);

    //tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    printf("Table size:%d\n",neuronToNodeTable->numberOfItems);

    NeuronEntry* neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&inputNeuron0);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[0]));
    TEST_ASSERT(neuronEntry->layer == 0);
    TEST_ASSERT(neuronEntry->indexInLayer == 0);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&inputNeuron1);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(isIPEqual(neuronEntry->nodeIP,nodes[1]));
    TEST_ASSERT(neuronEntry->layer == 0);
    TEST_ASSERT(neuronEntry->indexInLayer == 1);
    /******/

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();
}


void test_coordinator_handle_ACK_from_all_neurons(){
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    //In this test the ACK from the neurons 8 and 9 are missing
    char receivedMessage[50],ackMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;

    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;


    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    // NN_ACK [Acknowledge Neuron Id 1] [Acknowledge Neuron Id 2] [Acknowledge Neuron Id 3] ...
    snprintf(ackMessage, sizeof(ackMessage),"%d 2 3",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 4 5",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);

    snprintf(ackMessage, sizeof(ackMessage),"%d 6 7",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 8 9",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 10 11",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    NeuronEntry* neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron2);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron3);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron4);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron5);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron6);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron7);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron8);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron9);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron10);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);/******/

    neuronEntry = (NeuronEntry*) tableRead(neuronToNodeTable,&neuron11);
    TEST_ASSERT(neuronEntry != nullptr);
    TEST_ASSERT(neuronEntry->isAcknowledged);

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();

}

void test_coordinator_handle_ACK_missing_worker_neurons_on_ack_timeout(){
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    char receivedMessage[50],ackMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;

    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;


    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    // NN_ACK [Acknowledge Neuron Id 1] [Acknowledge Neuron Id 2] [Acknowledge Neuron Id 3] ...
    snprintf(ackMessage, sizeof(ackMessage),"%d 2 3",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 4 5",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 6 7",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 10 11",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    //tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUT_TARGETS, 2,neuron8,neuron9,
             1,myAPIP[0],myAPIP[1],myAPIP[3],myAPIP[3]);

    coordinator.onACKTimeOut(nodes,4);

    //printf("Encoded Message:%s\n",appPayload);
    //printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,appPayload) == 0);

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();

}

void test_coordinator_handle_ACK_missing_for_some_worker_neuron_of_same_node_on_ack_timeout(){
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    //In this test neuron 9 is not acknowledged
    char receivedMessage[50],ackMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;

    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;

    //initNeuralNetwork();
    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    // NN_ACK [Acknowledge Neuron Id 1] [Acknowledge Neuron Id 2] [Acknowledge Neuron Id 3] ...
    snprintf(ackMessage, sizeof(ackMessage),"%d 2 3",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 4 5",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 6 7",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 8",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 10 11",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    //tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUT_TARGETS, 1,neuron9,
             1,myAPIP[0],myAPIP[1],myAPIP[3],myAPIP[3]);

    coordinator.onACKTimeOut(nodes,4);

    //printf("Encoded Message:%s\n",appPayload);
    //printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,appPayload) == 0);

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();

}


void test_coordinator_handle_ACK_missing_input_neurons_on_ack_timeout(){
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    //In this test the neuron 1 is not acknowledge by node 3.3.3.3
    char receivedMessage[50],ackMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t inputNeuron0=0,inputNeuron1=1;
    float outputValue = 5.0;

    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;

    //initNeuralNetwork();
    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    coordinator.distributeInputNeurons(nodes,4);

    tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    // NN_ACK [Acknowledge Neuron Id 1] [Acknowledge Neuron Id 2] [Acknowledge Neuron Id 3] ...
    snprintf(ackMessage, sizeof(ackMessage),"%d 0",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    coordinator.onACKTimeOutInputLayer();

    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUT_TARGETS, 1,inputNeuron1,
             2,nodes[0][0],nodes[0][1],nodes[0][3],nodes[0][3],
            nodes[1][0],nodes[1][1],nodes[1][3],nodes[1][3]);

    coordinator.onACKTimeOutInputLayer();

    //printf("Encoded Message:%s\n",appPayload);
    //printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,appPayload) == 0);

    tableClean(neuronToNodeTable);
    //freeAllNeuronMemory();
}

void test_coordinator_assign_computations_on_ack_timeout(){
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    //In this test the neuron 2,3 is not acknowledge by node 2.2.2.2

    char receivedMessage[50],ackMessage[50];
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    char correctMessage[50];
    int pubTopic=1,subTopic=0;
    uint8_t neuron2=2,neuron3=3;
    float outputValue = 5.0;

    uint8_t nodes[4][4] = {
            {2,2,2,2},
            {3,3,3,3},
            {4,4,4,4},
            {5,5,5,5},
    };

    uint8_t neuron4=4,neuron5=5,neuron6=6,neuron7=7,neuron8=8,neuron9=9,neuron10=10,neuron11=11;

    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes,4);

    // NN_ACK [Acknowledge Neuron Id 1] [Acknowledge Neuron Id 2] [Acknowledge Neuron Id 3] ...

    snprintf(ackMessage, sizeof(ackMessage),"%d 3",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 4 5",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 6 7",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 8 9",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    snprintf(ackMessage, sizeof(ackMessage),"%d 10 11",NN_ACK);
    coordinator.handleNeuralNetworkMessage(blankIP,blankIP,ackMessage);//Assign neuron computation to the node

    //tablePrint(neuronToNodeTable,printNeuronTableHeader,printNeuronEntry);

    assignComputationsToNeuronsWithMissingAcks(nodes[0]);

    //|[Neuron ID] [Input Size] [Input Save Order] [weights values] [bias]
    snprintf(correctMessage, sizeof(correctMessage),"%d |2 2 0 1 0.5 -0.2 0.1", NN_ASSIGN_COMPUTATION);

    printf("Encoded Message:%s\n",appPayload);
    printf("Correct Message:%s\n",correctMessage);
    TEST_ASSERT(strcmp(correctMessage,appPayload) == 0);

    //freeAllNeuronMemory();
    tableClean(neuronToNodeTable);
}


void test_assign_outputs() {
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    char correctMessage[100];
    uint8_t nodes[4][4] = {
            {2, 2, 2, 2},
            {3, 3, 3, 3},
            {4, 4, 4, 4},
            {5, 5, 5, 5},
    };

    uint8_t neuron2 = 2, neuron3 = 3, neuron4 = 4, neuron5 = 5, neuron6 = 6, neuron7 = 7, neuron8 = 8, neuron9 = 9;

    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes, 4);
    //assignOutputTargetsToNetwork(nodes,4);

    coordinator.assignOutputTargetsToNode(appPayload, sizeof(appPayload),nodes[0]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUT_TARGETS, 2,neuron2,neuron3,
             2,nodes[2][0],nodes[2][1],nodes[2][3],nodes[2][3]
             ,nodes[3][0],nodes[3][1],nodes[3][3],nodes[3][3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);

    coordinator.assignOutputTargetsToNode(appPayload, sizeof(appPayload),nodes[1]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",NN_ASSIGN_OUTPUT_TARGETS,2,neuron4,neuron5
           ,2,nodes[2][0],nodes[2][1],nodes[2][3],nodes[2][3]
            ,nodes[3][0],nodes[3][1],nodes[3][3],nodes[3][3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);

    coordinator.assignOutputTargetsToNode(appPayload, sizeof(appPayload),nodes[2]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu",NN_ASSIGN_OUTPUT_TARGETS,2,neuron6,neuron7
            ,1,myAPIP[0],myAPIP[1],myAPIP[3],myAPIP[3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);/******/

    coordinator.assignOutputTargetsToNode(appPayload, sizeof(appPayload),nodes[3]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu.%hhu.%hhu.%hhu", NN_ASSIGN_OUTPUT_TARGETS,2,neuron8,neuron9
            ,1,myAPIP[0],myAPIP[1],myAPIP[3],myAPIP[3]);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);

    //freeAllNeuronMemory();
    tableClean(neuronToNodeTable);

}



void test_assign_pubsub_info() {
    NeuralNetworkCoordinator coordinator;
    uint8_t blankIP[4]={0,0,0,0};
    char correctMessage[100];
    uint8_t nodes[4][4] = {
            {2, 2, 2, 2},
            {3, 3, 3, 3},
            {4, 4, 4, 4},
            {5, 5, 5, 5},
    };

    uint8_t neuron2 = 2, neuron3 = 3, neuron4 = 4, neuron5 = 5, neuron6 = 6, neuron7 = 7, neuron8 = 8, neuron9 = 9;

    //initNeuralNetwork();
    coordinator.distributeNeuralNetwork(&neuralNetwork, nodes, 4);
    //assignOutputTargetsToNetwork(nodes,4);

    // |[Number of Neurons] [neuron ID1] [neuron ID2] [Subscription 1] [Pub 1]
    coordinator.assignPubSubInfoToNode(appPayload, sizeof(appPayload),nodes[0]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUT_TARGETS,2, neuron2,neuron3,0,1);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);

    coordinator.assignPubSubInfoToNode(appPayload, sizeof(appPayload),nodes[1]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUT_TARGETS,2, neuron4,neuron5,0,1);
    //printf("Correct Message:%s\n",correctMessage);
    //printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);

    coordinator.assignPubSubInfoToNode(appPayload, sizeof(appPayload),nodes[2]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUT_TARGETS,2, neuron6,neuron7,1,2);
    //printf("Correct Message:%s\n",correctMessage);
    //printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);

    coordinator.assignPubSubInfoToNode(appPayload, sizeof(appPayload),nodes[3]);
    snprintf(correctMessage, sizeof(correctMessage),"%d |%hhu %hhu %hhu %hhu %hhu",NN_ASSIGN_OUTPUT_TARGETS,2, neuron8,neuron9,1,2);
    printf("Correct Message:%s\n",correctMessage);
    printf("Encoded Message:%s\n",appPayload);
    TEST_ASSERT(strcmp(appPayload,correctMessage) == 0);/******/

    //freeAllNeuronMemory();
    tableClean(neuronToNodeTable);
}

void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    uint8_t IP[4] = {0,0,0,0};
    assignIP(myAPIP,IP);
}

void tearDown(void){
    strcpy(appPayload,"");
    strcpy(appBuffer,"");
   // tableClean(neuronToNodeTable);
}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_memory_allocation);
    RUN_TEST(test_neuron_output_calculation);

    RUN_TEST(test_handle_message_assign_neuron_one_neuron);
    RUN_TEST(test_handle_message_assign_neuron_multiple_neurons);
    RUN_TEST(test_handle_message_assign_neuron_with_more_than_max_neurons);
    RUN_TEST(test_handle_neuron_input);
    RUN_TEST(test_handle_assign_output_targets);/******/
    RUN_TEST(test_handle_assign_output_neuron);
    RUN_TEST(test_handle_assign_output_targets_to_not_handled_neuron);
    RUN_TEST(test_handle_assign_output_targets_multiple_layer_neurons);
    RUN_TEST(test_handle_assign_pubsub_info);
    RUN_TEST(test_handle_NACK_without_computed_output);
    RUN_TEST(test_handle_NACK_with_computed_output);
    RUN_TEST(test_handle_NACK_for_input_node);

    RUN_TEST(test_handle_assign_input_neuron_and_worker_neurons_and_assign_all_outputs);
    RUN_TEST(test_worker_neurons_from_multiple_layers_assigned);
    RUN_TEST(test_worker_compute_neuron_output_having_other_neurons_depending_on_that_output);
    RUN_TEST(test_worker_input_node_producing_output_needed_by_other_worker);

    //RUN_TEST(test_distribute_neural_network_to_one_device);

    RUN_TEST(test_encode_message_assign_neuron);
    RUN_TEST(test_encode_message_neuron_output);
    RUN_TEST(test_encode_assign_input_message);
    RUN_TEST(test_encode_NACK_message);
    RUN_TEST(test_encode_ACK_message);

    RUN_TEST(test_bit_fields);
    RUN_TEST(test_distribute_neurons);
    RUN_TEST(test_distribute_neurons_with_balanced_algorithm);
    RUN_TEST(test_assign_input_neurons);
    RUN_TEST(test_coordinator_handle_ACK_from_all_neurons);
    RUN_TEST(test_coordinator_handle_ACK_missing_worker_neurons_on_ack_timeout);
    RUN_TEST(test_coordinator_handle_ACK_missing_for_some_worker_neuron_of_same_node_on_ack_timeout);
    RUN_TEST(test_coordinator_handle_ACK_missing_input_neurons_on_ack_timeout);
    RUN_TEST(test_coordinator_assign_computations_on_ack_timeout);
    RUN_TEST(test_assign_outputs);
    RUN_TEST(test_assign_pubsub_info);/*** ***/

    UNITY_END();
}
