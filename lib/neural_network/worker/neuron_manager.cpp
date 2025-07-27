#include "neuron_manager.h"

// Variables used to track when the NACK mechanism was triggered, to manage this neuron's output in the presence of missing inputs
unsigned long nackTriggerTime;
bool nackTriggered = false;

// Timestamp marking the start of the current forward propagation cycle
unsigned long firstInputTimestamp;
// Variable to track if a NN forward pass is currently running
bool forwardPassRunning = false;

// Indicates whether all outputs of the neurons owned by this node have been computed
bool allOutputsComputed = false;
        
/*** Each node has a bitfield that indicates which inputs have been received during the current forward propagation
  *  step of the neural network.receivedInputs[neuronIndex][inputIndex] == 1 means the input was received. ***/
BitField receivedInputs[MAX_NEURONS] = {0};

// Store the output value of each neuron
float outputValues[MAX_NEURONS];

// Indicates whether the output of a given neuron has already been computed
bool isOutputComputed[MAX_NEURONS]={ false};

// To store the target nodes of each neuron output
OutputTarget outputTargets[MAX_NEURONS];

// Identifier of the current inference cycle, assigned by the root node
int currentInferenceId = 0;


/**
 * handleNeuronMessage
 * Processes all incoming neural network neuron related messages
 *
 * @param messageBuffer - Buffer containing the received message
 */
void handleNeuronMessage(char* messageBuffer){
    NeuralNetworkMessageType type;

    sscanf(messageBuffer, "%d",&type);

    switch (type) {
        case NN_ASSIGN_COMPUTATION:
            handleAssignComputationsMessage(messageBuffer);
            break;

        case NN_ASSIGN_OUTPUTS:
            handleAssignOutput(messageBuffer);
            break;

        case NN_NEURON_OUTPUT:
            //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
            handleNeuronOutputMessage(messageBuffer);
            break;

        case NN_FORWARD:
            //DATA_MESSAGE NN_FORWARD
            handleForwardMessage(messageBuffer);
            break;

        case NN_NACK:
            //DATA_MESSAGE NN_NACK  [Missing Output ID 1] [Missing Output ID 2] ...
            handleNACKMessage(messageBuffer);
            break;
        case NN_ACK:
            //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...

            break;

        default:
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
void handleAssignComputationsMessage(char*messageBuffer){
    uint8_t inputSize;
    NeuronId neuronID,*inputIndexMap;
    float bias, *weightValues;
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;

    //NN_ASSIGN_COMPUTATION [destinationIP] |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while (neuronEntry != nullptr){
        //LOG(NETWORK,DEBUG," neuron entry token:%s\n",neuronEntry);

        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ",&saveptr2);

        //spaceToken now pointing to the Neuron number
        neuronID = atoi(spaceToken);
        //LOG(NETWORK,DEBUG," neuronId token:%s\n",spaceToken);

        //spaceToken now pointing to the input size
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        inputSize = atoi(spaceToken);
        //LOG(NETWORK,DEBUG," inputSize token:%s\n",spaceToken);

        inputIndexMap = new NeuronId [inputSize];
        weightValues = new float[inputSize];

        //spaceToken now pointing to the Input Save Order Vector
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < inputSize; ++i) {
            inputIndexMap[i]= atoi(spaceToken);
            //LOG(NETWORK,DEBUG," inputIndexMap token:%s\n",spaceToken);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the weights Vector
        for (int i = 0; i < inputSize; ++i) {
            weightValues[i]=atof(spaceToken);
            //LOG(NETWORK,DEBUG," weightValues token:%s\n",spaceToken);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the bias value
        bias=atof(spaceToken);
        //LOG(NETWORK,DEBUG," bias token:%s\n",spaceToken);

        //Save the parsed neuron parameters
        configureNeuron(neuronID,inputSize,weightValues,bias, inputIndexMap);

        delete[] inputIndexMap;
        delete[] weightValues;

        //Move on to the next neuron
        neuronEntry = strtok_r(NULL, "|",&saveptr1);
    }
}


/**
 * handleAssignOutput
 * Processes NN_ASSIGN_OUTPUT messages to configure output targets for neurons (i.e., specifying which
 * nodes the output values should be sent to)
 *
 * @param messageBuffer - Buffer containing output assignment data
 * Format: |[Number of neurons] [Output Neuron IDs...] [Number of targets] [Target IPs...]
 */
void handleAssignOutput(char* messageBuffer){
    uint8_t nNeurons,nTargets,targetIP[4];
    NeuronId neuronID[MAX_NEURONS];
    char tmpBuffer[10];
    size_t tmpBufferSize = sizeof(tmpBuffer);
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;
    int offset=0;

    //Encode the message header of the ACK message
    offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%d ",NN_ACK);


    //DATA_MESSAGE NN_ASSIGN_OUTPUT |[Number of neurons] [Output Neuron ID 1] [Output Neuron ID 2]...[Number of targets] [Target IP 1] [Target IP 2]... |
    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while (neuronEntry != nullptr){
        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ",&saveptr2);

        //spaceToken now pointing to the number of Neurons
        nNeurons = atoi(spaceToken);
        //LOG(NETWORK,DEBUG," number of neurons: %hhu\n",nNeurons);

        //spaceToken now pointing to the list of neuron Ids
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < nNeurons; i++) {
            neuronID[i]= atoi(spaceToken);
            //LOG(NETWORK,DEBUG," neuronID: %hhu\n",neuronID[i]);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the number of Neurons
        nTargets = atoi(spaceToken);
        //LOG(NETWORK,DEBUG,"Nr targets: %hhu\n",nTargets);

        //spaceToken now pointing to the list of target nodes
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < nTargets; i++) {
            sscanf(spaceToken,"%hhu.%hhu.%hhu.%hhu",&targetIP[0],&targetIP[1],&targetIP[2],&targetIP[3]);
            //LOG(NETWORK,DEBUG,"IP: %hhu.%hhu.%hhu.%hhu\n",targetIP[0],targetIP[1],targetIP[2],targetIP[3]);
            // Update the list of target nodes associated with the parsed neurons
            updateOutputTargets(nNeurons, neuronID, targetIP);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        for (int i = 0; i < nNeurons; i++) {
            encodeACKMessage(tmpBuffer,tmpBufferSize,neuronID,nNeurons);
            offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%s",tmpBuffer);
        }

        //Move on to the next layer neurons
        neuronEntry = strtok_r(NULL, "|",&saveptr1);
    }

    //TODO send the ACK to the root node
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
void handleAssignPubSubInfo(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    uint8_t nNeurons,neuronID[MAX_NEURONS];
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;
    int pubTopic,subTopic;
    // |[Number of Neurons] [neuron ID1] [neuron ID2] [Subscription 1] [Pub 1]

    neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
    //Discard the message types
    neuronEntry = strtok_r(NULL, "|",&saveptr1);

    while (neuronEntry != nullptr) {
        // Position the spaceToken pointer at the beginning of the current neuron's data segment
        spaceToken = strtok_r(neuronEntry, " ", &saveptr2);

        //spaceToken now pointing to the number of Neurons
        nNeurons = atoi(spaceToken);
        //LOG(NETWORK, DEBUG, " number of neurons: %hhu\n", nNeurons);

        //spaceToken now pointing to the list of neuron Ids
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        for (int i = 0; i < nNeurons; i++) {
            neuronID[i] = atoi(spaceToken);
            //LOG(NETWORK, DEBUG, " neuronID: %hhu\n", neuronID[i]);
            //Move on the next input to index map
            spaceToken = strtok_r(NULL, " ", &saveptr2);
        }

        //spaceToken now pointing to the subTopic
        subTopic = atoi(spaceToken);
        //LOG(NETWORK, DEBUG, "subTopic: %i\n", subTopic);

        //spaceToken now pointing to the pubTopic
        spaceToken = strtok_r(NULL, " ", &saveptr2);
        pubTopic = atoi(spaceToken);
        //LOG(NETWORK, DEBUG, "pubTopic: %i\n", pubTopic);

        //Move on to the next layer neurons
        neuronEntry = strtok_r(NULL, "|", &saveptr1);

    }
}


/**
 * handleForwardMessage
 * Processes the message that initiates the forward propagation of the neural network,
 * initializing the relevant variables and storing the current inferenceId set by the coordinator.
 *
 * @param messageBuffer - Buffer containing forward pass command
 */
void handleForwardMessage(char *messageBuffer){
    int inferenceId;
    sscanf(messageBuffer, "%*d %i",&inferenceId);

    currentInferenceId = inferenceId;

    for (int i = 0; i < neuronsCount; i++) {
        resetAll(receivedInputs[i]);
        isOutputComputed[i] = false;
    }
    allOutputsComputed = false;

    forwardPassRunning = true;
    firstInputTimestamp = getCurrentTime();

    nackTriggered = false;

}

/**
 * handleNACKMessage
 * Processes NN_NACK messages that indicate missing neuron outputs.
 * Parses all neuron IDs from the message and if any correspond to neurons computed by this node,
 * sends their output values (if already available).
 *
 * @param messageBuffer - Buffer containing NACK information
 * Format: [Neuron ID with Missing Output 1] [Neuron ID with Missing Output 2]...
 */
void handleNACKMessage(char*messageBuffer){
    //NN_NACK [Neuron ID with Missing Output 1] [Neuron ID with Missing Output 2] ...
    char *saveptr1, *token;
    NeuronId currentId;
    int neuronStorageIndex = -1;
    bool ownsNeuronInNack=false;
    token = strtok_r(messageBuffer, " ",&saveptr1);

    //Skip the message header
    token = strtok_r(NULL, " ",&saveptr1);

    while(token != nullptr){
        // Extract the ID of the neuron whose output is currently missing
        currentId = atoi(token);

        LOG(APP,DEBUG,"NACK neuronID: %hhu\n",currentId);

        neuronStorageIndex = getNeuronStorageIndex(currentId);

        LOG(APP,DEBUG,"neuron storage index: %d\n",neuronStorageIndex);


        // If this node manages the neuron in the NACK, process the NACK
        // If the output is not yet computed, the node will send it to the respective destinations once it is
        if(neuronStorageIndex != -1) {
            // If the output is already computed, resend it to the neuron that missed it
            LOG(APP,DEBUG,"The output is computed?: %d\n",isOutputComputed[neuronStorageIndex]);

            if(isOutputComputed[neuronStorageIndex]){
                //Encode the message containing the neuron output value
                encodeNeuronOutputMessage(appPayload,sizeof(appPayload),currentId,outputValues[neuronStorageIndex]);
                //TODO send the message
            }

            ownsNeuronInNack = true;
        }

        token = strtok_r(NULL, " ",&saveptr1);
    }

    //TODO broadCast the NACK: With our without my computed neurons

}


/**
 * handleNeuronOutputMessage
 * Processes NN_NEURON_OUTPUT messages containing computed neuron output values that serve as inputs for neurons on this node.
 * Stores the inputs for the relevant neurons, and if all required inputs have arrived, computes the corresponding outputs and sends them.
 *
 * @param messageBuffer - Buffer containing neuron output data
 * Format: [Inference Id] [Output Neuron ID] [Output Value]
 */
void handleNeuronOutputMessage(char*messageBuffer){
    int inputStorageIndex = -1, neuronStorageIndex = -1, inputSize = -1, currentNeuronID = 0, inferenceId;
    float neuronOutput, inputValue;
    bool outputsComputed=true;
    NeuronId outputNeuronId;

    sscanf(messageBuffer, "%*d %i %hhu %f",&inferenceId,&outputNeuronId,&inputValue);

    /***If the inferenceId received in the message is lower than the current inferenceId, the message belongs to
     * an outdated inference cycle and should be discarded ***/
    if(inferenceId < currentInferenceId) return;
    /*** If the inferenceId from another neuron's output message is greater than the one currently stored,
     it means this node's sequence is outdated, possibly due to missing the coordinator's message
     that signaled the start of the forward propagation cycle.***/
    else if(inferenceId > currentNeuronID){
        currentInferenceId = inferenceId;
        // Also reset other variables that should have been reset when the new forward message arrived
        forwardPassRunning = true;
        allOutputsComputed = false;
        firstInputTimestamp = getCurrentTime();
    }

    for (int i = 0; i < neuronsCount; i++) {
        currentNeuronID = neuronIds[i];

        // Check if the current neuron requires the input produced by outputNeuronId
       if(isInputRequired(currentNeuronID,outputNeuronId)){
           // Find the index where the neuron is stored
           neuronStorageIndex = getNeuronStorageIndex(currentNeuronID);
           // Find the storage index of this specific input value for the given neuron
           inputStorageIndex = getInputStorageIndex(currentNeuronID,outputNeuronId);
           //Find the input size of the neuron
           inputSize = getInputSize(currentNeuronID);

           if(inputSize == -1 || inputStorageIndex == -1 || neuronStorageIndex == -1){
               LOG(APP,ERROR,"ERROR: Invalid index detected: inputSize=%d, inputStorageIndex=%d, neuronStorageIndex=%d",
                   inputSize, inputStorageIndex, neuronStorageIndex);
               return;
           }

           //Save the input value in the input vector
           setInput(currentNeuronID,inputValue,outputNeuronId);

           // Set the bit corresponding to the received input to 1
           setBit(receivedInputs[neuronStorageIndex],inputStorageIndex);

           // Check if all inputs required by that specific neuron have been received
           if(allBits(receivedInputs[neuronStorageIndex], inputSize)){
               // If all inputs required by the neuron have been received, proceed with output computation
               neuronOutput = computeNeuronOutput(currentNeuronID);
               outputValues[neuronStorageIndex] = neuronOutput;

               //LOG(APP,DEBUG,"Output inside function:%f\n",neuronOutput);
               //reset the bit field for the next NN run
               resetAll(receivedInputs[neuronStorageIndex]);//TODO PASS THIS FOR WHE THE FOWARD MESSAGE IS RECEIVED

               isOutputComputed[neuronStorageIndex] = true;
               //TODO Send the output for the nodes that need him
           }
       }
    }

    // Check whether the newly received input value completes the set of missing inputs.
    for (int i = 0; i < neuronsCount; i++) {
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
void updateOutputTargets(uint8_t nNeurons, uint8_t *neuronId, uint8_t targetIP[4]){
    int neuronStorageIndex = -1;
    for (int i = 0; i < nNeurons; i++) {
        // For each neuron in the provided list, determine where it should be stored
        neuronStorageIndex = getNeuronStorageIndex(neuronId[i]);

        //Skit if the neuron is not managed by this node
        if(neuronStorageIndex == -1)continue;

        // Skip if the targetIP is already in the list of target devices
        if(isIPinList(targetIP,outputTargets[neuronStorageIndex].outputTargets,outputTargets[neuronStorageIndex].nTargets)){
            continue;
        }
        // Add the new targetIP to the first available slot in the list
        for (int j = 0; j < 4; ++j) {
            outputTargets[neuronStorageIndex].outputTargets[outputTargets[neuronStorageIndex].nTargets][j] = targetIP[j];
        }
        //Increment the number of target nodes
        outputTargets[neuronStorageIndex].nTargets++;
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
void encodeNeuronOutputMessage(char* messageBuffer,size_t bufferSize,NeuronId outputNeuronId, float neuronOutput){
    int offset = 0;
    //NN_NEURON_OUTPUT [Inference Id] [Output Neuron ID] [Output Value]
    //Encode the neuron id that generated this output
    offset = snprintf(messageBuffer,bufferSize,"%d %i %d ",NN_NEURON_OUTPUT,currentInferenceId,outputNeuronId);

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
void encodeNACKMessage(char* messageBuffer, size_t bufferSize,NeuronId  missingNeuron){
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
void encodeACKMessage(char* messageBuffer, size_t bufferSize,NeuronId *neuronAckList, int ackNeuronCount){
    int offset = 0;
    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...

    //offset = snprintf(messageBuffer,bufferSize,"%d ",NN_ACK);

    // Encode the IDs of neurons whose required inputs are missing
    for (int i = 0; i < ackNeuronCount; i++) {
        offset += snprintf(messageBuffer+offset,bufferSize-offset,"%hhu ",neuronAckList[i]);
    }

}

//TODO Header
void encodeWorkerRegistration(char* messageBuffer, size_t bufferSize,uint8_t nodeIP[4],DeviceType type){
    //NN_WORKER_REGISTRATION [Node IP] [Device Type]
    snprintf(messageBuffer,bufferSize,"%hhu.%hhu.%hhu.%hhu %d",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],static_cast<int>(type));
}



/**
 * manageNeuron
 * Main neuron management function that handles NeuralNetwork timeouts.
 * Should be called regularly in the main loop.
 */
void manageNeuron(){
    unsigned long currentTime = getCurrentTime();

    // Check if the expected time for input arrival has already passed
    if((currentTime-firstInputTimestamp) >= INPUT_WAIT_TIMEOUT && forwardPassRunning && !allOutputsComputed){
        onInputWaitTimeout();
    }

    // Check if any sent NACKs have timed out, meaning the corresponding inputs should have arrived by now but didn’t
    if((currentTime - nackTriggerTime) >= NACK_TIMEOUT  &&  nackTriggered){
        onNACKTimeout();
    }
}

/**
 * onInputWaitTimeout
 * Handles situations where not all inputs arrive before the established timeout.
 * Triggers NACK messages containing the IDs of the missing input neurons.
 */
void onInputWaitTimeout(){
    int neuronStorageIndex = -1,offset=0;
    uint8_t inputSize = -1;
    char tmpBuffer[20];
    size_t tmpBufferSize = sizeof(tmpBuffer);

    // The message aggregates the missing inputs from all neurons into a single message
    offset += snprintf(appPayload+offset, sizeof(appPayload)-offset,"%d ",NN_NACK);

    // Iterate through all neurons to determine which have incomplete input sets and identify the missing inputs
    // Since the IDs of neurons missing inputs are irrelevant to the nodes providing those inputs, the neuron IDs are not included in the message.
    for (int i = 0; i < neuronsCount; i++) {

        neuronStorageIndex = getNeuronStorageIndex(neuronIds[i]);
        if(neuronStorageIndex == -1) continue;

        inputSize = inputSizes[neuronStorageIndex];

        // Check whether each neuron has received all its expected inputs and thus computed its output
        if(!isOutputComputed[neuronStorageIndex]){
            // Identify inputs that have not yet been received
            for (int j = 0; j < inputSize; j++) {
                // If the bit is not set, it means the input has not been received yet
                if(!isBitSet(receivedInputs[neuronStorageIndex],j)){
                    encodeNACKMessage(tmpBuffer, tmpBufferSize,saveOrders[neuronStorageIndex][j]);
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
void onNACKTimeout(){
    float outputValue;
    // Search for outputs that have not been computed yet and compute them, filling in any missing inputs with the values from the last inference.
    for (int i = 0; i < neuronsCount; i++) {
        if(!isOutputComputed[i]){
            //Compute the neuron Output value
            outputValue = computeNeuronOutput(neuronIds[i]);
            outputValues[i] = outputValue;

            //Mark the output as computed
            isOutputComputed[i] = true;
            //TODO Send the output for the nodes that need him

            //reset the bit field for the next NN run
            resetAll(receivedInputs[i]);
        }
    }

    allOutputsComputed = true;
    nackTriggered = false;

    // If all outputs have been computed, the forward pass has ended at this node.
     forwardPassRunning = false;
}