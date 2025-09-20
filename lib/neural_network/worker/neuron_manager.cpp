#include "neuron_manager.h"

//#include <Arduino.h>

float sensorData = 0.0;

/**
 * handleNeuronMessage
 * Processes all incoming neural network neuron related messages
 *
 * @param messageBuffer - Buffer containing the received message
 */
void NeuronWorker::handleNeuronMessage(uint8_t* senderIP,uint8_t* destinationIP,char* messageBuffer){
    NeuralNetworkMessageType type;
    uint8_t originatorIP[4],finalDestination[4];
    char tmpPayload[30];

    bool encapsulatedMessage = network.isDataMessageEncapsulated(messageBuffer);
    // NN_NEURON_OUTPUT messages when using the Strategy Inject are encapsulated within data messages.
    // Therefore, the payload must be parsed first in order to access them.
    if(network.getActiveMiddlewareStrategy() ==STRATEGY_INJECT && encapsulatedMessage){
        network.parseDataMessage(messageBuffer,originatorIP,senderIP,tmpPayload, sizeof(tmpPayload));
        sscanf(tmpPayload, "%d",&type);
    }else{
        sscanf(messageBuffer, "%d",&type);
    }

    switch (type) {
        case NN_ASSIGN_COMPUTATION:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_ASSIGN_COMPUTATION);
            LOG(APP,INFO,"Received [NN_ASSIGN_COMPUTATION] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleAssignComputationsMessage(messageBuffer);
            break;

        case NN_ASSIGN_INPUT:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_ASSIGN_INPUT);
            LOG(APP,INFO,"Received [NN_ASSIGN_INPUT] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleAssignInput(messageBuffer);
            break;

        case NN_ASSIGN_OUTPUT:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_ASSIGN_OUTPUT);
            LOG(APP,INFO,"Received [NN_ASSIGN_OUTPUT] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            handleAssignOutputNeuron(messageBuffer);
            break;

        case NN_ASSIGN_OUTPUT_TARGETS:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_ASSIGN_OUTPUT_TARGETS);
            LOG(APP,INFO,"Received [NN_ASSIGN_OUTPUT_TARGETS] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY || network.getActiveMiddlewareStrategy()==STRATEGY_INJECT) {
                handleAssignOutputTargets(messageBuffer);
            }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                handleAssignPubSubInfo(messageBuffer);
            }
            break;

        case NN_NEURON_OUTPUT:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_NEURON_OUTPUT);
            if(network.getActiveMiddlewareStrategy()==STRATEGY_INJECT && encapsulatedMessage){
                LOG(APP,INFO,"Received [NN_NEURON_OUTPUT] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                        ,tmpPayload,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
                handleNeuronOutputMessage(tmpPayload);
            }else{
                LOG(APP,INFO,"Received [NN_NEURON_OUTPUT] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                        ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
                // NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
                handleNeuronOutputMessage(messageBuffer);
            }

            break;

        case NN_FORWARD:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_FORWARD);
            LOG(APP,INFO,"Received [NN_FORWARD] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            //DATA_MESSAGE NN_FORWARD
            handleForwardMessage(messageBuffer);
            break;

        case NN_NACK:
            monitoring.reportDataMessageReceived(sizeof(messageBuffer),9,NN_NACK);

            LOG(APP,INFO,"Received [NN_NACK] message: \"%s\" from %hhu.%hhu.%hhu.%hhu\n"
                    ,messageBuffer,senderIP[0],senderIP[1],senderIP[2],senderIP[3]);
            //DATA_MESSAGE NN_NACK [Missing Output ID 1] [Missing Output ID 2] ...
            handleNACKMessage(messageBuffer,senderIP);
            break;

        default:
            LOG(APP,ERROR,"This node received a message meant for the coordinator node. Message: %s", messageBuffer);
            break;
    }
}


/**
 * handleAssignComputationsMessage
 * Processes NN_ASSIGN_COMPUTATION messages to configure neuron parameters (neuronId,inputs,weights and biases)
 *
 * @param messageBuffer - Buffer containing computation assignment data
 * Format: |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
 */
void NeuronWorker::handleAssignComputationsMessage(char*messageBuffer){
    uint8_t inputSize;
    NeuronId neuronID,*inputIndexMap;
    float bias, *weightValues;
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;
    int messageOffset=0;


    //NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while (neuronEntry != nullptr){
        //LOG(APP,DEBUG," neuron entry token:%s\n",neuronEntry);

        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ",&saveptr2);

        //spaceToken now pointing to the Neuron number
        neuronID = atoi(spaceToken);
        //LOG(APP,DEBUG," neuronId token:%s\n",spaceToken);

        //spaceToken now pointing to the input size
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        inputSize = atoi(spaceToken);
        //LOG(APP,DEBUG," inputSize token:%s\n",spaceToken);

        inputIndexMap = new NeuronId [inputSize];
        weightValues = new float[inputSize];

        //spaceToken now pointing to the Input Save Order Vector
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < inputSize; ++i) {
            inputIndexMap[i]= atoi(spaceToken);
            //LOG(APP,DEBUG," inputIndexMap token:%s\n",spaceToken);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the weights Vector
        for (int i = 0; i < inputSize; ++i) {
            weightValues[i]=atof(spaceToken);
            //LOG(APP,DEBUG," weightValues token:%s\n",spaceToken);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the bias value
        bias=atof(spaceToken);
        //LOG(APP,DEBUG," bias token:%s\n",spaceToken);

        // Save the parsed neuron parameters if the neuron is not already in this node's list of computed neurons
        if(!neuronCore.computesNeuron(neuronID))neuronCore.configureNeuron(neuronID,inputSize,weightValues,bias, inputIndexMap);

        delete[] inputIndexMap;
        delete[] weightValues;

        //Move on to the next neuron
        neuronEntry = strtok_r(NULL, "|",&saveptr1);
    }
    neuronCore.printNeuronInfo();
}


/**
 * handleAssignOutputTargets
 * Processes NN_ASSIGN_OUTPUT messages to configure output targets for neurons (i.e., specifying which
 * nodes the output values should be sent to)
 *
 * @param messageBuffer - Buffer containing output assignment data
 * Format: |[Number of neurons] [Output Neuron IDs...] [Number of targets] [Target IPs...]
 */
void NeuronWorker::handleAssignOutputTargets(char* messageBuffer){
    uint8_t nNeurons,nTargets,targetIP[4],nComputedNeurons=0;
    NeuronId neuronID[MAX_NEURONS], currentNeuronId;
    char tmpBuffer[10];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;
    int offset=0;

    //Encode the message header of the ACK message
    offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%d",NN_ACK);


    //DATA_MESSAGE NN_ASSIGN_OUTPUT |[Number of neurons] [Output Neuron ID 1] [Output Neuron ID 2]...[Number of targets] [Target IP 1] [Target IP 2]... |
    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while (neuronEntry != nullptr){
        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ",&saveptr2);

        //spaceToken now pointing to the number of Neurons
        nNeurons = atoi(spaceToken);
        //LOG(APP,DEBUG," number of neurons: %hhu\n",nNeurons);

        //spaceToken now pointing to the list of neuron Ids
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < nNeurons; i++) {
            currentNeuronId= atoi(spaceToken);
            /*** If the neuron targeted by this assignment is not computed by this node, it won't be added to the list
             *  of neurons to acknowledge. This implies that a message containing the node assignments was lost,
             *  so by not acknowledging the neuron, the root will resend the assignment. ***/
            if(neuronCore.computesNeuron(currentNeuronId) || isNeuronInList(inputNeurons,nrInputNeurons,currentNeuronId)){
                neuronID[nComputedNeurons] = currentNeuronId;
                nComputedNeurons ++;
                //LOG(APP,DEBUG,"NeuronId: %hhu\n",currentNeuronId);
            }
            //LOG(APP,DEBUG," neuronID: %hhu\n",neuronID[i]);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the number of Neurons
        nTargets = atoi(spaceToken);
        //LOG(APP,DEBUG,"Nr targets: %hhu\n",nTargets);

        //spaceToken now pointing to the list of target nodes
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < nTargets; i++) {
            sscanf(spaceToken,"%hhu.%hhu.%hhu.%hhu",&targetIP[0],&targetIP[1],&targetIP[2],&targetIP[3]);
            //LOG(APP,DEBUG,"IP: %hhu.%hhu.%hhu.%hhu\n",targetIP[0],targetIP[1],targetIP[2],targetIP[3]);
            // Update the list of target nodes associated with the parsed neurons
            updateOutputTargets(nComputedNeurons, neuronID, targetIP);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        encodeACKMessage(tmpBuffer,tmpBufferSize,neuronID,nComputedNeurons);
        offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%s",tmpBuffer);

        //Move on to the next layer neurons
        neuronEntry = strtok_r(NULL, "|",&saveptr1);
        nComputedNeurons=0;
    }

    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);
}

/**
 * handleAssignInput
 * Processes messages from the coordinator node assigning this node a input neuron
 *
 * @param messageBuffer - Buffer containing the input neuron data
 * Format: NN_ASSIGN_INPUT [neuronID]
 */
void NeuronWorker::handleAssignInput(char* messageBuffer){
    NeuronId inputNeuronId;
    //NN_ASSIGN_INPUT [neuronID]
    sscanf(messageBuffer, "%*d %hhu",&inputNeuronId);

    // Only save the input neuron assignment if the neuron is not already assigned to this node.
    /***if(!isNeuronInList(inputNeurons,nrInputNeurons,inputNeuronId)){
        inputNeurons[nrInputNeurons] = inputNeuronId;
        nrInputNeurons++;
    }***/

    // If the device has reached the maximum number of supported neurons, no additional input neurons can be stored
    if(nrInputNeurons>=MAX_INPUT_NEURONS){
        LOG(APP,ERROR,"The number of assigned input neurons exceeds the allowed maximum. Neuron ID: %hhu was not accepted",inputNeuronId);
        return;
    }

    //If the input neuron inst in the list add it (this safeguards against multiple messages assign the same input neuron)
    if(!isNeuronInList(inputNeurons,nrInputNeurons,inputNeuronId)){
        inputNeurons[nrInputNeurons] = inputNeuronId;
        nrInputNeurons++;
    }

    //todo
    //inputNeuronAssignmentCallback(input Neuron Id)
}

void NeuronWorker::handleAssignOutputNeuron(char* messageBuffer){
    uint8_t inputSize;
    NeuronId neuronID,*inputIndexMap;
    float bias, *weightValues;
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;
    int offset=0;
    char tmpBuffer[10];
    size_t tmpBufferSize = sizeof(tmpBuffer);

    //Encode the message header of the ACK message
    offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%d",NN_ACK);

    //NN_ASSIGN_OUTPUT |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias] | ...
    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while (neuronEntry != nullptr){
        //LOG(APP,DEBUG," neuron entry token:%s\n",neuronEntry);

        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ",&saveptr2);

        //spaceToken now pointing to the Neuron number
        neuronID = atoi(spaceToken);
        //LOG(APP,DEBUG," neuronId token:%s\n",spaceToken);

        //spaceToken now pointing to the input size
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        inputSize = atoi(spaceToken);
        //LOG(APP,DEBUG," inputSize token:%s\n",spaceToken);

        inputIndexMap = new NeuronId [inputSize];
        weightValues = new float[inputSize];

        //spaceToken now pointing to the Input Save Order Vector
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < inputSize; ++i) {
            inputIndexMap[i]= atoi(spaceToken);
            //LOG(APP,DEBUG," inputIndexMap token:%s\n",spaceToken);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the weights Vector
        for (int i = 0; i < inputSize; ++i) {
            weightValues[i]=atof(spaceToken);
            //LOG(APP,DEBUG," weightValues token:%s\n",spaceToken);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the bias value
        bias=atof(spaceToken);
        //LOG(APP,DEBUG," bias token:%s\n",spaceToken);

        // Store the parsed neuron parameters if they have not been saved yet
        if(!neuronCore.computesNeuron(neuronID)){
            neuronCore.configureNeuron(neuronID,inputSize,weightValues,bias, inputIndexMap);
        }

        // If the device has reached the maximum number of supported neurons, no additional output neurons can be stored
        if(nrOutputNeurons>=MAX_NEURONS){
            LOG(APP,ERROR,"The number of assigned output neurons exceeds the allowed maximum. Neuron ID: %hhu was not accepted",neuronID);
        }else{
            //If the output neuron inst in the list add it
            if(!isNeuronInList(outputNeurons,nrOutputNeurons,neuronID)){
                outputNeurons[nrOutputNeurons] = neuronID;
                nrOutputNeurons++;
            }

        }

        //Add the parsed neuronID to the NACK message
        encodeACKMessage(tmpBuffer,tmpBufferSize,&neuronID,1);
        offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%s",tmpBuffer);

        delete[] inputIndexMap;
        delete[] weightValues;

        //Move on to the next neuron
        neuronEntry = strtok_r(NULL, "|",&saveptr1);
    }

    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);
}

/**
 * handleAssignPubSubInfo
 * Processes messages from the coordinator node containing Pub/Sub topic assignments
 * related to the PubSub strategy for this node.
 *
 * @param messageBuffer - Buffer containing pub/sub data
 * Format: |[Number of Neurons] [neuron IDs...] [Subscription] [Publication]
 */
void NeuronWorker::handleAssignPubSubInfo(char* messageBuffer){
    uint8_t nNeurons, nComputedNeurons=0;
    NeuronId neuronID[MAX_NEURONS], currentNeuronId;
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;
    int pubTopic,subTopic,neuronStorageIndex,offset=0;
    char tmpBuffer[10];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    int8_t subTopics[MAX_NEURONS],pubTopics[MAX_NEURONS+1];
    int nSubTopics=0,nPubTopics=0;

    // |[Number of Neurons] [neuron ID1] [neuron ID2] [Subscription 1] [Pub 1]

    //Encode the message header of the ACK message
    offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%d",NN_ACK);

    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while(neuronEntry != nullptr){
        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ", &saveptr2);

        //spaceToken now pointing to the number of Neurons
        nNeurons = atoi(spaceToken);
        //LOG(APP, DEBUG, " number of neurons: %hhu\n", nNeurons);

        //spaceToken now pointing to the list of neuron Ids
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < nNeurons; i++) {
            currentNeuronId = atoi(spaceToken);
            /*** If the neuron targeted by this assignment is not computed by this node, it won't be added to the list
            *  of neurons to acknowledge. This implies that a message containing the node assignments was lost,
            *  so by not acknowledging the neuron, the root will resend the assignment. ***/
            if(neuronCore.computesNeuron(currentNeuronId) || isNeuronInList(inputNeurons,nrInputNeurons,currentNeuronId)){
                neuronID[nComputedNeurons] = currentNeuronId;
                nComputedNeurons ++;
                //LOG(APP,DEBUG,"NeuronId: %hhu\n",currentNeuronId);
            }
            //LOG(APP, DEBUG, " neuronID: %hhu\n", neuronID[i]);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the subTopic
        subTopic = atoi(spaceToken);
        LOG(APP, DEBUG, "Subscribing to topic: %i\n", subTopic);
        /*** Subscribe to the topic if it is valid (not equal to -1). A value of -1 indicates an invalid or non-existent
         *   topic, so the node should not subscribe.If the topic is already in the subscription list, avoid adding it again.***/
        if(subTopic != -1 && !isTopicInList(subTopics,nSubTopics,subTopic)){
            if(nSubTopics<MAX_NEURONS ){//todo and is not already in the list
                subTopics[nSubTopics]=static_cast<int8_t>(subTopic);
                nSubTopics++;
            }
        }
        //network.subscribeToTopic(static_cast<int8_t>(subTopic));

        //spaceToken now pointing to the pubTopic
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        pubTopic = atoi(spaceToken);

        // Publish the topic if it is valid (not equal to -1).
        if(pubTopic != -1){
            LOG(APP, DEBUG, "Publishing topic: %i\n", pubTopic);
            // If the topic is not equal to -1, publish it
            if(nSubTopics<MAX_NEURONS && !isTopicInList(pubTopics,nPubTopics,pubTopic)){
                pubTopics[nPubTopics]=static_cast<int8_t>(pubTopic);
                nPubTopics++;
            }
            //network.advertiseTopic(static_cast<int8_t>(pubTopic));

            // For neurons that publish this topic, store the mapping between the published topic and the corresponding neuron.
            for (int i = 0; i < nNeurons; i++) {
                neuronStorageIndex = neuronCore.getNeuronStorageIndex(neuronID[i]);
                // Record the topic published by this neuron in the neuron-to-topic map
                if(neuronStorageIndex != -1) neuronToTopicMap[neuronStorageIndex]= static_cast<int8_t>(pubTopic);
            }
        }

        //LOG(APP, DEBUG, "pubTopic: %i\n", pubTopic);
        encodeACKMessage(tmpBuffer,tmpBufferSize,neuronID,nComputedNeurons);
        offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%s",tmpBuffer);
        //Move on to the next layer neurons
        neuronEntry = strtok_r(NULL, "|", &saveptr1);
        nComputedNeurons=0;
    }
    // Call the middleware function to update the pub/sub information's
    network.subscribeAndPublishTopics(subTopics,nSubTopics,pubTopics,nPubTopics);
    //network.middlewarePrintInfo();

    //delay(1000);

    // Send message with the acknowledged neurons to the root
    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);

}


/**
 * clearNeuronInferenceParameters
 * Resets all neuron state variables to prepare for a new inference cycle.
 */
void NeuronWorker::clearNeuronInferenceParameters() {
    for (int i = 0; i < neuronCore.neuronsCount; i++) {
        resetAll(receivedInputs[i]);
        isOutputComputed[i] = false;
    }
    //LOG(APP,DEBUG,"F2\n");

    allOutputsComputed = false;

    forwardPassRunning = true;
    firstInputTimestamp = getCurrentTime();

    nackTriggered = false;
}

/**
 * handleForwardMessage
 * Processes the message that initiates the forward propagation of the neural network,
 * initializing the relevant variables and storing the current inferenceId set by the coordinator.
 *
 * @param messageBuffer - Buffer containing forward pass command
 */
void NeuronWorker::handleForwardMessage(char *messageBuffer){
    int inferenceId;
    sscanf(messageBuffer, "%*d %i",&inferenceId);

    //If the device isn't responsible for any input, hidden/output neurons return
    if(neuronCore.neuronsCount==0 && nrInputNeurons ==0) return;

    currentInferenceId = inferenceId;

    clearNeuronInferenceParameters();

    //LOG(APP,DEBUG,"F3\n");
    //Generate the input of all input neurons hosted in this node
    for (int i = 0; i < nrInputNeurons; i++) {
        generateInputData(inputNeurons[i]);
    }

}

/**
 * handleNACKMessage
 * Processes NN_NACK messages that indicate missing neuron outputs.
 * Parses all neuron IDs from the message and if any correspond to neurons computed by this node,
 * sends their output values (if already available).
 *
 * @param messageBuffer - Buffer containing NACK information
 * @param senderIP - IP address of the generator of the NACK message
 * Format: [Neuron ID with Missing Output 1] [Neuron ID with Missing Output 2]...
 */
void NeuronWorker::handleNACKMessage(char*messageBuffer,uint8_t *senderIP){
    //NN_NACK [Neuron ID with Missing Output 1] [Neuron ID with Missing Output 2] ...
    char *saveptr1, *token;
    NeuronId currentId;
    int neuronStorageIndex = -1,inputNeuronIndex;
    char tmpNACKBuffer[100];

    strncpy(tmpNACKBuffer,messageBuffer, sizeof(tmpNACKBuffer)-1);
    tmpNACKBuffer[sizeof(tmpNACKBuffer) - 1] = '\0';

    token = strtok_r(tmpNACKBuffer, " ",&saveptr1);

    //Skip the message header
    token = strtok_r(NULL, " ",&saveptr1);

    while(token != nullptr){
        // Extract the ID of the neuron whose output is currently missing
        currentId = atoi(token);

        //LOG(APP,DEBUG,"NACK neuronID: %hhu\n",currentId);
        neuronStorageIndex = neuronCore.getNeuronStorageIndex(currentId);

        //LOG(APP,DEBUG,"neuron storage index: %d\n",neuronStorageIndex);

        // If this node manages the neuron in the NACK, process the NACK
        // If the output is not yet computed, the node will send it to the respective destinations once it is
        if(neuronStorageIndex != -1) {
            // If the output is already computed, resend it to the neuron that missed it
            LOG(APP,DEBUG,"The output is computed?: %d\n",isOutputComputed[neuronStorageIndex]);

            if(isOutputComputed[neuronStorageIndex]){
                //Encode the message containing the neuron output value
                encodeNeuronOutputMessage(appPayload,sizeof(appPayload),currentInferenceId,currentId,outputValues[neuronStorageIndex]);
                //TODO send the message
                network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,senderIP);
            }
        }

        // If the neuron in the NACK is an input neuron managed by this node send the input value
        if(isNeuronInList(inputNeurons,nrInputNeurons,currentId)){
            // Search for the index where the input neuron ID is stored
            inputNeuronIndex= getInputNeuronStorageIndex(currentId);

            // The inputNeuronIndex indicates where the neuron's input value is stored in the inputValues vector
            encodeNeuronOutputMessage(appPayload,sizeof(appPayload),currentInferenceId,currentId,inputNeuronsValues[inputNeuronIndex]);
            //Send the message containing the neuron output value to the device that misses that
            network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,senderIP);
        }

        token = strtok_r(NULL, " ",&saveptr1);
    }

    nackCount++;

}


/**
 * handleNeuronOutputMessage
 * Processes NN_NEURON_OUTPUT messages containing computed neuron output values that serve as inputs for neurons on this node.
 * Stores the inputs for the relevant neurons, and if all required inputs have arrived, computes the corresponding outputs and sends them.
 *
 * @param messageBuffer - Buffer containing neuron output data
 * Format: [Inference Id] [Output Neuron ID] [Output Value]
 */
void NeuronWorker::handleNeuronOutputMessage(char*messageBuffer){
    int inferenceId;
    float inputValue;
    NeuronId outputNeuronId;

    sscanf(messageBuffer, "%*d %i %hhu %f",&inferenceId,&outputNeuronId,&inputValue);

    processNeuronInput(outputNeuronId,inferenceId,inputValue);

}

//TODO header
void NeuronWorker::processNeuronInput(NeuronId inputNeuronId,int inferenceId,float inputValue){
    int inputStorageIndex = -1, neuronStorageIndex = -1, inputSize = -1;
    float neuronOutput;
    bool outputsComputed=true,neuronsRequireInput=false;
    NeuronId currentNeuronID = 0,handledNeuronId;
    uint8_t myIP[4], rootIP[4];

    //Get the node IP
    network.getNodeIP(myIP);

    /***If the inferenceId received in the message is lower than the current inferenceId, the message belongs to
     * an outdated inference cycle and should be discarded ***/
    if(inferenceId < currentInferenceId) return;
    /*** If the inferenceId from another neuron's output message is greater than the one currently stored,
     it means this node's sequence is outdated, possibly due to missing the coordinator's message
     that signaled the start of the forward propagation cycle.***/
    else if(inferenceId > currentInferenceId){
        currentInferenceId = inferenceId;
        // Also reset other variables that should have been reset when the new forward message arrived
        clearNeuronInferenceParameters();
    }

    for (int i = 0; i < neuronCore.neuronsCount; i++) {


        currentNeuronID = neuronCore.getNeuronId(i);

        // Check if the current neuron requires the input produced by outputNeuronId
        if(neuronCore.isInputRequired(currentNeuronID,inputNeuronId)){

            /*** Skip this neuron if its output is already computed. This prevents duplicate computations
             when an NN_OUTPUT message arrives in response to a NACK, but the output was already
             calculated locally due to the NACK timeout.***/
            if(isOutputComputed[neuronStorageIndex])continue;
            // Find the index where the neuron is stored
            neuronStorageIndex = neuronCore.getNeuronStorageIndex(currentNeuronID);
            // Find the storage index of this specific input value for the given neuron
            inputStorageIndex = neuronCore.getInputStorageIndex(currentNeuronID,inputNeuronId);
            //Find the input size of the neuron
            inputSize = neuronCore.getInputSize(currentNeuronID);

            if(inputSize == -1 || inputStorageIndex == -1 || neuronStorageIndex == -1){
                LOG(APP,ERROR,"ERROR: Invalid index detected: inputSize=%d, inputStorageIndex=%d, neuronStorageIndex=%d",
                    inputSize, inputStorageIndex, neuronStorageIndex);
                return;
            }


            //Save the input value in the input vector
            neuronCore.setInput(currentNeuronID,inputValue,inputNeuronId);

            // Set the bit corresponding to the received input to 1
            setBit(receivedInputs[neuronStorageIndex],inputStorageIndex);

            // Check if all inputs required by that specific neuron have been received
            if(allBits(receivedInputs[neuronStorageIndex], inputSize)){

                // If all inputs required by the neuron have been received, proceed with output computation
                neuronOutput = neuronCore.computeNeuronOutput(currentNeuronID);
                outputValues[neuronStorageIndex] = neuronOutput;

                LOG(APP,INFO,"Neuron %hhu generated output: %f\n",currentNeuronID,neuronOutput);

                //Iterates through all neurons managed by this node to identify which require the computed output as their input
                for (int j = 0; j < neuronCore.neuronsCount; j++) {
                    handledNeuronId = neuronCore.getNeuronId(j);
                    if(handledNeuronId != 255 && neuronCore.isInputRequired(handledNeuronId,currentNeuronID)){
                        neuronsRequireInput=true;
                        break;
                    }
                }

                // If any node requires the computed output, feed it to them by recursively calling this function
                if(neuronsRequireInput)processNeuronInput(currentNeuronID,inferenceId,neuronOutput);

                //reset the bit field for the next NN run
                resetAll(receivedInputs[neuronStorageIndex]);//TODO PASS THIS FOR WHE THE FOWARD MESSAGE IS RECEIVED

                isOutputComputed[neuronStorageIndex] = true;


                // Encode the message with the neuron output
                encodeNeuronOutputMessage(appPayload, sizeof(appPayload),currentInferenceId,currentNeuronID,neuronOutput);
                if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY){
                    // Send the computed neuron output value to every target node that need it
                    for (int j = 0; j < neuronTargets[neuronStorageIndex].nTargets; j++) {
                        // If the target node is this node, there is no need to send the message to myself
                        // since the neuron's output can be directly fed into the local input without transmission.
                        if(isIPEqual(myIP,neuronTargets[neuronStorageIndex].targetsIPs[j])) continue;
                        network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,neuronTargets[neuronStorageIndex].targetsIPs[j]);
                    }
                }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                    // With the pub/sub strategy simply call the function to influence routing and the middleware will deal with who needs the output
                    network.influenceRoutingStrategyPubSub(appBuffer, sizeof(appBuffer),appPayload);
                }else if(network.getActiveMiddlewareStrategy()==STRATEGY_INJECT){
                    network.getRootIP(rootIP);
                    // The output undergoes influence routing only if it requires further computation,
                    // redirecting it through a node with greater processing capacity
                    if(neuronTargets[neuronStorageIndex].nTargets > 0){
                        network.influenceRoutingStrategyInject(appBuffer, sizeof(appBuffer),appPayload,rootIP);
                    }
                }

                //If the Neuron is a Neuron from the Output layer do something
                if(isNeuronInList(outputNeurons,MAX_NEURONS,currentNeuronID)) onNeuralNetworkOutput(currentNeuronID,neuronOutput);

                //If the Neuron whose output have been computed is an output neurons do something
                if(isNeuronInList(outputNeurons,nrOutputNeurons,currentNeuronID)){
                    if(onOutputLayerComputation)onOutputLayerComputation(currentNeuronID,neuronOutput);
                }

            }
        }
    }

    // Check whether the newly received input value completes the set of missing inputs.
    for (int i = 0; i < neuronCore.neuronsCount; i++) {
        outputsComputed = isOutputComputed[i] && outputsComputed;
    }

    // If the NACK mechanism was triggered but all missing inputs have now arrived (and all outputs are computed),the NACK process can be stopped.
    if(nackTriggered) nackTriggered = outputsComputed;

    allOutputsComputed = outputsComputed;

    // If all outputs have been computed, the forward pass has ended at this node.
    if(allOutputsComputed) forwardPassRunning = false;

}
/**
 * updateOutputTargets
 * Updates the list of target nodes for specified output neurons.
 *
 * @param nNeurons - Number of neurons to update
 * @param neuronId - Array of neuron IDs
 * @param targetIP - IP address of the target node (IPv4 format)
 */
void NeuronWorker::updateOutputTargets(uint8_t nNeurons, uint8_t *neuronId, uint8_t targetIP[4]){
    int neuronStorageIndex = -1;

    for (int i = 0; i < nNeurons; i++) {
        //Verify if the current neuron is an input neuron
        if(isNeuronInList(inputNeurons,nrInputNeurons,neuronId[i])){
            // If the number of targets exceeds the available storage space, skip the target (don't store it in memory) and log the error
            if(inputTargets.nTargets == MAX_TARGET_OUTPUTS){
                LOG(APP, ERROR, "Error: Number of output target nodes exceeded the configured maximum for Neuron ID: %hhu\n", neuronId[i]);
                continue;
            }

            // Skip if the targetIP is already in the list of target devices of the input neurons
            if(isIPinList(targetIP,inputTargets.targetsIPs,inputTargets.nTargets)){
                continue;
            }

            // Add the new targetIP to the first available slot in the list
            for (int j = 0; j < 4; j++) {
                inputTargets.targetsIPs[inputTargets.nTargets][j] = targetIP[j];
            }

            //Increment the number of target nodes
            inputTargets.nTargets++;

        }else{// If the neuron is one of the computing neurons handled by this node
            // For each neuron in the provided list, determine where it should be stored
            neuronStorageIndex = neuronCore.getNeuronStorageIndex(neuronId[i]);

            //Skit if the neuron is not managed by this node
            if(neuronStorageIndex == -1)continue;

            // If the number of targets exceeds the available storage space, skip the target (don't store it in memory) and log the error
            if(neuronTargets[neuronStorageIndex].nTargets == MAX_TARGET_OUTPUTS){
                LOG(APP, ERROR, "Error: Number of output target nodes exceeded the configured maximum for Neuron ID: %hhu\n", neuronId[i]);
                continue;
            }

            // Skip if the targetIP is already in the list of target devices
            if(isIPinList(targetIP,neuronTargets[neuronStorageIndex].targetsIPs,neuronTargets[neuronStorageIndex].nTargets)){
                continue;
            }

            // Add the new targetIP to the first available slot in the list
            for (int j = 0; j < 4; j++) {
                neuronTargets[neuronStorageIndex].targetsIPs[neuronTargets[neuronStorageIndex].nTargets][j] = targetIP[j];
            }
            //Increment the number of target nodes
            neuronTargets[neuronStorageIndex].nTargets++;
        }


    }
}

void NeuronWorker::clearNeuronOutputTargets(NeuronId neuronId){
    int neuronStorageIndex = -1;

    // If the neuron is an input neuron, its input targets are stored in a separate structure.
    if(isNeuronInList(inputNeurons,nrInputNeurons,neuronId)){
        // There is no need to clear the target IP values, only the number of targets,
        // because the list is iterated according to the number of targets.
        inputTargets.nTargets=0;
    }else if(neuronCore.computesNeuron(neuronId)){
        neuronStorageIndex=neuronCore.getNeuronStorageIndex(neuronId);
        if(neuronStorageIndex==-1)return;
        neuronTargets[neuronStorageIndex].nTargets=0;
    }
}


/**
 * encodeNeuronOutputMessage
 * Formats a message containing a neuron's output value.
 *
 * @param messageBuffer - Output buffer for the message
 * @param bufferSize - Size of the output buffer
 * @param outputNeuronId - ID of the neuron producing the output
 * @param neuronOutput - Computed output value
 */
void NeuronWorker::encodeNeuronOutputMessage(char* messageBuffer,size_t bufferSize,int inferenceId,NeuronId outputNeuronId, float neuronOutput){
    int offset = 0;
    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    //Encode the neuron id that generated this output
    offset = snprintf(messageBuffer,bufferSize,"%d %i %d ",NN_NEURON_OUTPUT,inferenceId,outputNeuronId);

    // Encode the computed output value
    snprintf(messageBuffer+offset,bufferSize-offset,"%g",neuronOutput);

}


/**
 * encodeNACKMessage
 * Formats a NACK message containing the IDs of neurons whose output values are missing.
 *
 * @param messageBuffer - Output buffer for the message
 * @param bufferSize - Size of the output buffer
 * @param missingNeuron - ID of the neuron with missing output
 */
void NeuronWorker::encodeNACKMessage(char* messageBuffer, size_t bufferSize,NeuronId  missingNeuron){
    int offset = 0;
    //DATA_MESSAGE NN_NACK [Neuron ID with Missing Output] [Missing Output ID 1] [Missing Output ID 2] ...

    //offset = snprintf(messageBuffer,bufferSize,"%d",NN_NACK);

    // Encode the missing input
    offset += snprintf(messageBuffer+offset,bufferSize-offset," %d",missingNeuron);
}


/**
 * encodeACKMessage
 * Formats an ACK message acknowledging the specified neurons.
 *
 * @param messageBuffer - Output buffer for the message
 * @param bufferSize - Size of the output buffer
 * @param neuronAckList - Array of acknowledged neuron IDs
 * @param ackNeuronCount - Number of neurons in the ACK list
 */
void NeuronWorker::encodeACKMessage(char* messageBuffer, size_t bufferSize,NeuronId *neuronAckList, int ackNeuronCount){
    int offset = 0;
    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...

    //offset = snprintf(messageBuffer,bufferSize,"%d ",NN_ACK);
    //LOG(APP,DEBUG,"ACK count:%hhu\n",ackNeuronCount);

    // Encode the IDs of neurons whose required inputs are missing
    for (int i = 0; i < ackNeuronCount; i++) {
        //LOG(APP,DEBUG,"ACK neuron:%hhu\n",neuronAckList[i]);
        offset += snprintf(messageBuffer+offset,bufferSize-offset," %hhu",neuronAckList[i]);
    }

}



//TODO Header
void NeuronWorker::encodeWorkerRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4],DeviceType type){
    //NN_WORKER_REGISTRATION [Node IP] [Device Type]
    snprintf(messageBuffer,bufferSize,"%d %hhu.%hhu.%hhu.%hhu %d",NN_WORKER_REGISTRATION,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],static_cast<int>(type));
}
//TODO Header
void NeuronWorker::encodeInputRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4],DeviceType type){
    //NN_INPUT_REGISTRATION [Node IP] [Device Type]
    snprintf(messageBuffer,bufferSize,"%d %hhu.%hhu.%hhu.%hhu %d",NN_INPUT_REGISTRATION,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],static_cast<int>(type));
}

//TODO Header
void NeuronWorker::encodeOutputRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4]){
    //NN_OUTPUT_REGISTRATION [Node IP]
    snprintf(messageBuffer,bufferSize,"%d %hhu.%hhu.%hhu.%hhu",NN_OUTPUT_REGISTRATION,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
}

void NeuronWorker::generateInputData(NeuronId inputNeuronId){
    uint8_t myLocalIP[4];
    int inputNeuronStorageIndex=-1;
    sensorData += 1.0;

    //sensorData = inputGenerationCallback(inputNeuronId);

    //LOG(APP,DEBUG,"F4\n");
    inputNeuronStorageIndex = getInputNeuronStorageIndex(inputNeuronId);
    if(inputNeuronStorageIndex==-1) {
        LOG(APP,ERROR,"ERROR: Neuron %hhu is not registered as an input\n",inputNeuronId);
        return;
    }

    //LOG(APP,DEBUG,"F5\n");

    inputNeuronsValues[inputNeuronStorageIndex] = sensorData;

    //LOG(APP,DEBUG,"F6\n");

    // If this node is responsible for computing neurons that depend on this input, provide it directly to them.
    processNeuronInput(inputNeuronId,currentInferenceId,sensorData);

    //LOG(APP,DEBUG,"F7\n");

    network.getNodeIP(myLocalIP);

    //LOG(APP,DEBUG,"F8\n");

    // Encode the message to send to other nodes, containing my output value that serves as their input.
    encodeNeuronOutputMessage(appPayload, sizeof(appPayload),currentInferenceId,inputNeuronId,sensorData);
    if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY){
        // Send the generated input data to each one of the targets
        for (int j = 0; j < inputTargets.nTargets; j++) {
            // If this node hosts a neuron that depends on locally generated input, skip it there's no need to send a
            // message to itself since the input is already available.
            if(isIPEqual(myLocalIP,inputTargets.targetsIPs[j])) continue;
            network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,inputTargets.targetsIPs[j]);
        }
    }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
        // With the pub/sub strategy simply call the function to influence routing and the middleware will deal with who needs the output
        network.influenceRoutingStrategyPubSub(appBuffer, sizeof(appBuffer),appPayload);
    }else if(network.getActiveMiddlewareStrategy()==STRATEGY_INJECT){
        uint8_t rootIP[4];
        network.getRootIP(rootIP);
        // The output undergoes influence routing only if it requires further computation,
        // redirecting it through a node with greater processing capacity
        if(inputTargets.nTargets > 0){
            // Apply influence routing to pass the values through a higher-capacity device, with the root node as the final recipient.
            network.influenceRoutingStrategyInject(appBuffer, sizeof(appBuffer),appPayload,rootIP);
        }
    }
}


/**
 * manageNeuron
 * Main neuron management function that handles NeuralNetwork timeouts.
 * Should be called regularly in the main loop.
 */
void NeuronWorker::manageNeuron(){
    unsigned long currentTime = getCurrentTime();

    // Skip neuron management if a forward pass is not in progress
    if(!forwardPassRunning)return;

    /*** Verify two conditions before proceeding:
     1. The input wait timeout has elapsed
     2. Not all outputs have been computed yet
     3. The NACK mechanism hasn't been triggered yet
     Then initialize the onInputWaitTimeout procedure that sends NACKs related to the missing neurons ***/
    if(!allOutputsComputed && !nackTriggered && (currentTime-firstInputTimestamp) >= INPUT_WAIT_TIMEOUT ){
        LOG(APP,INFO,"Missing Input Values: starting the onInputWaitTimeOut process\n");
        onInputWaitTimeout();
    }

    currentTime = getCurrentTime();
    // Check if any sent NACKs have timed out, meaning the corresponding inputs should have arrived by now but did’t
    if((currentTime - nackTriggerTime) >= NACK_TIMEOUT  &&  nackTriggered){
        LOG(APP,INFO,"Missing Input Values, after sending NACKs: computing the missing neuron values\n");
        onNACKTimeout();
    }
}

/**
 * onInputWaitTimeout
 * Handles situations where not all inputs arrive before the established timeout.
 * Triggers NACK messages containing the IDs of the missing input neurons.
 */
void NeuronWorker::onInputWaitTimeout(){
    int neuronStorageIndex = -1,offset=0;
    uint8_t inputSize = -1, nMissingNeurons=0;
    char tmpBuffer[20];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    NeuronId missingNeuronId, handledNeuronId, missingNeuronsList[10];
    // The message aggregates the missing inputs from all neurons into a single message
    offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%d",NN_NACK);

    // Iterate through all neurons to determine which have incomplete input sets and identify the missing inputs
    // Since the IDs of neurons missing inputs are irrelevant to the nodes providing those inputs, the neuron IDs are not included in the message.
    for (int i = 0; i < neuronCore.neuronsCount; i++) {

        handledNeuronId = neuronCore.getNeuronId(i);
        if(handledNeuronId == 255) continue;

        neuronStorageIndex = neuronCore.getNeuronStorageIndex(handledNeuronId);
        if(neuronStorageIndex == -1) continue;

        inputSize = neuronCore.getInputSize(handledNeuronId);

        // Check whether each neuron has received all its expected inputs and thus computed its output
        if(!isOutputComputed[neuronStorageIndex]){
            // Identify inputs that have not yet been received
            for (int j = 0; j < inputSize; j++) {
                // If the bit is not set, it means the input has not been received yet
                if(!isBitSet(receivedInputs[neuronStorageIndex],j)){
                    missingNeuronId = neuronCore.getInputNeuronId(handledNeuronId,j);

                    // Don't include missing neurons handled by this node in the NACK. The neurons we manage feed their
                    // outputs directly to dependent neurons. If an output isn't provided, it means it's not yet available
                    if(isNeuronInList(inputNeurons,nrInputNeurons,missingNeuronId) ||neuronCore.computesNeuron(missingNeuronId)){
                        continue;
                    }

                    // Check if the missing neuron is already in the NACK list; if it is, there is no need to add it again.
                    if(isNeuronInList(missingNeuronsList,nMissingNeurons,missingNeuronId)){
                        continue;
                    }else{
                        if(nMissingNeurons<=10){
                            missingNeuronsList[nMissingNeurons] = missingNeuronId;
                            nMissingNeurons++;
                        }else{
                            LOG(APP,ERROR, "Error: Cannot add missing neuron ID %hhu, NACK list capacity (10) exceeded\n", missingNeuronId);
                        }
                    }
                    encodeNACKMessage(tmpBuffer, tmpBufferSize,missingNeuronId);
                    offset += snprintf(appPayload+offset,sizeof(appPayload)-offset,"%s",tmpBuffer);
                }
            }
        }
    }
    //TODO BroadCast the message to the network
    network.broadcastMessage(appBuffer, sizeof(appBuffer),appPayload);

    //Set up the NACK control variables
    nackTriggered = true;
    nackTriggerTime = getCurrentTime();

}

/**
 * onNACKTimeout
 * Handles the NACK timeout ,that is, when inputs do not arrive between the NACK trigger and the timeout limit.
 * In this case, the node proceeds with output calculations using the received inputs,
 * substituting any missing inputs with values from the previous iteration.
 */
void NeuronWorker::onNACKTimeout(){
    float outputValue;
    NeuronId handledNeuronId;
    int neuronStorageIndex=-1;
    uint8_t myIP[4];

    network.getNodeIP(myIP);
    // Search for outputs that have not been computed yet and compute them, filling in any missing inputs with the values from the last inference.
    for (int i = 0; i < neuronCore.neuronsCount; i++){
        if(!isOutputComputed[i]){
            handledNeuronId = neuronCore.getNeuronId(i);
            neuronStorageIndex =neuronCore.getNeuronStorageIndex(handledNeuronId);
            if(handledNeuronId == 255 || neuronStorageIndex == -1)continue; //skip invalid values

            //Compute the neuron Output value
            outputValue = neuronCore.computeNeuronOutput(handledNeuronId);
            outputValues[i] = outputValue;

            //Mark the output as computed
            isOutputComputed[i] = true;
            //TODO Send the output for the nodes that need him

            LOG(APP,INFO,"Neuron %hhu generated output: %f\n",handledNeuronId,outputValue);

            // Encode the message with the neuron output
            encodeNeuronOutputMessage(appPayload, sizeof(appPayload),currentInferenceId,handledNeuronId,outputValue);
            if(network.getActiveMiddlewareStrategy()==STRATEGY_NONE || network.getActiveMiddlewareStrategy()==STRATEGY_TOPOLOGY){
                // Send the computed neuron output value to every target node that need it
                for (int j = 0; j < neuronTargets[neuronStorageIndex].nTargets; j++) {
                    // If the target node is this node, there is no need to send the message to myself
                    // since the neuron's output can be directly fed into the local input without transmission.
                    if(isIPEqual(myIP,neuronTargets[neuronStorageIndex].targetsIPs[j])) continue;
                    network.sendMessageToNode(appBuffer, sizeof(appBuffer),appPayload,neuronTargets[neuronStorageIndex].targetsIPs[j]);
                }
            }else if(network.getActiveMiddlewareStrategy()==STRATEGY_PUBSUB){
                // With the pub/sub strategy simply call the function to influence routing and the middleware will deal with who needs the output
                network.influenceRoutingStrategyPubSub(appBuffer, sizeof(appBuffer),appPayload);
            } else if(network.getActiveMiddlewareStrategy()==STRATEGY_INJECT){
                uint8_t rootIP[4];
                network.getRootIP(rootIP);
                // The output undergoes influence routing only if it requires further computation,
                // redirecting it through a node with greater processing capacity
                if(neuronTargets[neuronStorageIndex].nTargets > 0){
                    // Apply influence routing to pass the values through a higher-capacity device, with the root node as the final recipient.
                    network.influenceRoutingStrategyInject(appBuffer, sizeof(appBuffer),appPayload,rootIP);
                }
            }

            //reset the bit field for the next NN run
            //resetAll(receivedInputs[i]);
        }
    }

    allOutputsComputed = true;
    nackTriggered = false;

    // If all outputs have been computed, the forward pass has ended at this node.
    forwardPassRunning = false;
}

void NeuronWorker::clearAllNeuronMemory(){
    uint8_t blankIP[4]={0,0,0,0};
    for (int i = 0; i < MAX_NEURONS; ++i) {
        resetAll(receivedInputs[i]);
        receivedInputs[i] = 0;
        outputValues[i]=0;
        isOutputComputed[i]= false;
        for (int j = 0; j < neuronTargets[i].nTargets; ++j) {
            assignIP(neuronTargets[i].targetsIPs[j],blankIP);
        }
        neuronTargets[i].nTargets=0;
    }
}


int NeuronWorker::getInputNeuronStorageIndex(NeuronId neuronId){
    for (int i = 0; i < nrInputNeurons; i++) {
        if (inputNeurons[i] == neuronId) return i;
    }
    return -1;
}

void NeuronWorker::registerNodeAsInput(){
    uint8_t myIP[4];
    network.getNodeIP(myIP);
    //encode the input registration message
    encodeInputRegistration(appPayload, sizeof(appPayload),myIP,deviceType);
    // Send the message to the neural network coordinator (the root node)
    LOG(APP,INFO,"Sending [NN_INPUT_REGISTRATION] message: \"%s\"\n",appPayload);
    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);
}

void NeuronWorker::registerNodeAsWorker() {
    uint8_t myIP[4];
    network.getNodeIP(myIP);
    //encode the input registration message
    encodeWorkerRegistration(appPayload, sizeof(appPayload),myIP,deviceType);
    LOG(APP,INFO,"Sending [NN_WORKER_REGISTRATION] message: \"%s\"\n",appPayload);
    // Send the message to the neural network coordinator (the root node)
    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);
}

void NeuronWorker::registerNodeAsOutput() {
    uint8_t myIP[4];
    network.getNodeIP(myIP);
    //encode the input registration message
    encodeOutputRegistration(appPayload, sizeof(appPayload),myIP);
    LOG(APP,INFO,"Sending [NN_OUTPUT_REGISTRATION] message: \"%s\"\n",appPayload);
    // Send the message to the neural network coordinator (the root node)
    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);
}

void NeuronWorker::decodeNeuronTopic(char* dataMessage, int8_t* topicType){
    NeuronId outputNeuronId;
    int neuronStorageIndex;
    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    //Remove the output neuron id from the message
    sscanf(dataMessage, "%*d %*i %hhu",&outputNeuronId);


    neuronStorageIndex=neuronCore.getNeuronStorageIndex(outputNeuronId);
    //If the neuron whose output in being produced is an input neuron then the topic type is equal to zero
    //input layer neurons publish the layer 0 topic
    if(isNeuronInList(inputNeurons,nrInputNeurons,outputNeuronId)){
        *topicType = 0;
        return;
    }else if(neuronStorageIndex != -1){
        // Topics published by the computing neurons are stored in the neuronToTopicMap
        *topicType = neuronToTopicMap[neuronStorageIndex];
        return;
    }
    *topicType = -1;

}

void NeuronWorker::saveOutputNeuron(NeuronId outputNeuronId) {
    // If the device has reached the maximum number of supported neurons, no additional output neurons can be stored
    if(nrOutputNeurons>=MAX_NEURONS){
        LOG(APP,ERROR,"The number of assigned output neurons exceeds the allowed maximum. Neuron ID: %hhu was not accepted",outputNeuronId);
    }else{
        outputNeurons[nrOutputNeurons] = outputNeuronId;
        nrOutputNeurons++;
    }
}

void NeuronWorker::saveInputNeuron(NeuronId inputNeuronId) {
    // If the device has reached the maximum number of supported neurons, no additional input neurons can be stored
    if(nrInputNeurons>=MAX_INPUT_NEURONS){
        LOG(APP,ERROR,"The number of assigned input neurons exceeds the allowed maximum. Neuron ID: %hhu was not accepted",inputNeuronId);
        return;
    }
    inputNeurons[nrInputNeurons] = inputNeuronId;
    nrInputNeurons++;
}

void NeuronWorker::saveWorkerTargets(NeuronId neuronId, uint8_t (*targetNodeIP)[4], uint8_t nTargets) {

}

void NeuronWorker::saveWorkerPubSubInfo(NeuronId neuronId, int8_t pubTopic) {

}

void NeuronWorker::onNeuralNetworkOutput(NeuronId neuronId, float outputValue) {
    uint8_t rootIP[4];
    network.getRootIP(rootIP);

    // Send inference results to the root node for aggregation and reporting to the monitoring server.
    LOG(APP,INFO,"/////// Inference Cycle Output - NeuronId:%hhu Output Value:%f //////////\n",neuronId,outputValue);
    encodeNeuronOutputMessage(appPayload, sizeof(appPayload),currentInferenceId,neuronId,outputValue);
    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);

}


bool isTopicInList(int8_t *topicList, int listSize, int8_t searchTopic){
    for (int i = 0; i < listSize; ++i) {
        if(topicList[i] == searchTopic) return true;
    }
    return false;
}

