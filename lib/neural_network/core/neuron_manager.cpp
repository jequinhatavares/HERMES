#include "neuron_manager.h"

unsigned long nackTriggerTime;
bool nackTriggered= false;

unsigned long firstInputTimestamp;
bool forwardPassRunning = false;

bool allInputsReceived = false;
        
/*** Each node has a bitfield that indicates which inputs have been received during the current forward propagation
  *  step of the neural network.receivedInputs[neuronIndex][inputIndex] == 1 means the input was received. ***/
BitField receivedInputs[MAX_NEURONS] = {0};

// Store the output value of each neuron
float outputValues[MAX_NEURONS];

// Indicates whether the output of a given neuron has already been computed
bool isOutputComputed[MAX_NEURONS]={ false};


OutputTarget outputTargets[MAX_NEURONS];

void handleNeuralNetworkMessage(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    uint8_t nNeurons,nTargets,inputSize;
    NeuronId neuronID,outputNeuron,*inputIndexMap;
    float bias, *weightValues,inputValue;
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;

    switch (type) {
        case NN_ASSIGN_COMPUTATION:
            //DATA_MESSAGE NN_ASSIGN_COMPUTATION [destinationIP] |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
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
            break;

        case NN_ASSIGN_OUTPUTS:
            handleAssignOutput(messageBuffer);
            break;

        case NN_NEURON_OUTPUT:
            //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
            sscanf(messageBuffer, "%*d %*d %hhu %f",&outputNeuron,&inputValue);
            handleNeuronInput(outputNeuron,inputValue);
            break;

        case NN_FORWARD:
            //DATA_MESSAGE NN_FORWARD
            break;

        case NN_NACK:
            //DATA_MESSAGE NN_NACK [Neuron ID with Missing Output] [Missing Output ID 1] [Missing Output ID 2] ...

            break;
        case NN_ACK:
            //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...

            break;

        default:
            break;
    }
}



void handleAssignOutput(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    uint8_t nNeurons,nTargets,targetIP[4];
    NeuronId neuronID[MAX_NEURONS];
    char *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;

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

        //Move on to the next layer neurons
        neuronEntry = strtok_r(NULL, "|",&saveptr1);
    }
}


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

void handleNACKMessage(char*messageBuffer){

}

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


void handleNeuronInput(int outputNeuronId, float inputValue){
    int inputStorageIndex = -1, neuronStorageIndex = -1, inputSize = -1, currentNeuronID = 0;
    float neuronOutput;

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
               resetAll(receivedInputs[neuronStorageIndex]);

               isOutputComputed[neuronStorageIndex] = true;
               //TODO Send the output for the nodes that need him
           }
       }
    }


}
void encodeMessageHeader(char* messageBuffer,size_t bufferSize,NeuralNetworkMessageType type){
    if(type == NN_NACK){
        snprintf(messageBuffer,bufferSize,"%d",NN_NACK);
    }
}
void encodeNeuronOutputMessage(char* messageBuffer,size_t bufferSize,NeuronId outputNeuronId, float neuronOutput){
    int offset = 0;
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Output Value]
    //Encode the neuron id that generated this output
    offset = snprintf(messageBuffer,bufferSize,"%d %d ",NN_NEURON_OUTPUT,outputNeuronId);

    // Encode the computed output value
    offset += snprintf(messageBuffer+offset,bufferSize-offset,"%g",neuronOutput);

}

void encodeNACKMessage(char* messageBuffer, size_t bufferSize,NeuronId  missingNeuron){
    int offset = 0;
    //DATA_MESSAGE NN_NACK [Neuron ID with Missing Output] [Missing Output ID 1] [Missing Output ID 2] ...

    //offset = snprintf(messageBuffer,bufferSize,"%d",NN_NACK);

    // Encode the missing input
    offset += snprintf(messageBuffer+offset,bufferSize-offset," %d",missingNeuron);
}

void encodeACKMessage(char* messageBuffer, size_t bufferSize,NeuronId *neuronAckList, int ackNeuronCount){
    int offset = 0;
    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...

    //offset = snprintf(messageBuffer,bufferSize,"%d ",NN_ACK);

    // Encode the IDs of neurons whose required inputs are missing
    for (int i = 0; i < ackNeuronCount; i++) {
        offset += snprintf(messageBuffer+offset,bufferSize-offset,"%hhu ",neuronAckList[i]);
    }

}


bool isIPinList(uint8_t *searchIP,uint8_t list[][4],uint8_t nElements){
    for (uint8_t i = 0; i < nElements; i++) {
        if(isIPEqual(list[i],searchIP)){
            return true;
        }
    }
    return false;
}

void manageNeuron(){
    unsigned long currentTime = getCurrentTime();

    // Check if the expected time for input arrival has already passed
    if((currentTime-firstInputTimestamp) >= INPUT_WAIT_TIMEOUT && forwardPassRunning && !allInputsReceived){
        onInputWaitTimeout();
    }

    // Check if any sent NACKs have timed out, meaning the corresponding inputs should have arrived by now but didn’t
    if((currentTime - nackTriggerTime) >= NACK_TIMEOUT  &&  nackTriggered){


        nackTriggered = false;
    }
}

void onInputWaitTimeout(){
    int neuronStorageIndex = -1,offset=0;
    uint8_t inputSize = -1;
    NeuronId neuronId;
    char tmpBuffer[20];
    size_t tmpBufferSize = sizeof(tmpBuffer),sendBufferSize=sizeof(smallSendBuffer);

    // The message aggregates the missing inputs from all neurons into a single message
    offset += snprintf(smallSendBuffer+offset,sendBufferSize-offset,"%d ",NN_NACK);

    // Iterate through all neurons to determine which have incomplete input sets and identify the missing inputs
    // Since the IDs of neurons missing inputs are irrelevant to the nodes providing those inputs, the neuron IDs are not included in the message.
    for (int i = 0; i < neuronsCount; i++) {

        neuronId = neuronIds[i];
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
                    offset += snprintf(smallSendBuffer+offset,sendBufferSize-offset,"%s",tmpBuffer);
                }
            }
        }
    }
    //TODO BroadCast the message to the network

    //Set up the NACK control variables
    nackTriggered = true;
    nackTriggerTime = getCurrentTime();

}

void onNackTimeout(){

}