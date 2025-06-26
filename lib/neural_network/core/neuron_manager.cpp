#include "neuron_manager.h"

BitField receivedInputs[MAX_NEURONS];

float outputValues[MAX_NEURONS];

void handleNeuralNetworkMessage(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    int neuronId,inputSize,*inputIndexMap,outputNeuron;
    float bias, *weightValues,inputValue;
    char* token, *spaceToken,*neuronEntry;
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
                neuronId = atoi(spaceToken);
                //LOG(NETWORK,DEBUG," neuronId token:%s\n",spaceToken);

                //spaceToken now pointing to the input size
                spaceToken = strtok_r(NULL, " ", &saveptr2);
                inputSize = atoi(spaceToken);
                //LOG(NETWORK,DEBUG," inputSize token:%s\n",spaceToken);

                inputIndexMap = new int[inputSize];
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
                configureNeuron(neuronId,inputSize,weightValues,bias, inputIndexMap);

                delete[] inputIndexMap;
                delete[] weightValues;

                //Move on to the next neuron
                neuronEntry = strtok_r(NULL, "|",&saveptr1);
            }
            break;

        case NN_ASSIGN_OUTPUTS:
            //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID 1] [Corresponding IP 1] [Output Neuron ID 2] [Corresponding IP 2]

            break;

        case NN_NEURON_OUTPUT:
            //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Input Neuron Number] [Output Value]
            sscanf(messageBuffer, "%*d %*d %d %d %f",&outputNeuron,&neuronId,&inputValue);
            handleNeuronInput(neuronId,outputNeuron,inputValue);
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

void handleNeuronInput(int neuronId,int outputNeuronId,float inputValue){
    int inputStorageIndex = -1, neuronStorageIndex = -1, inputSize = -1;
    float neuronOutput;

    // Find the index where the neuron is stored
    neuronStorageIndex = getNeuronStorageIndex(neuronId);
    // Find the storage index of this specific input value for the given neuron
    inputStorageIndex = getInputStorageIndex(neuronId,outputNeuronId);
    //Find the input size of the neuron
    inputSize = getInputSize(neuronId);

    if(inputSize == -1 || inputStorageIndex == -1 || neuronStorageIndex == -1){
        LOG(APP,ERROR,"ERROR: Invalid index detected: inputSize=%d, inputStorageIndex=%d, neuronStorageIndex=%d",
            inputSize, inputStorageIndex, neuronStorageIndex);
        return;
    }

    //Save the input value in the input vector
    setInput(neuronId,inputValue,outputNeuronId);

    // Set the bit corresponding to the received input to 1
    setBit(receivedInputs[neuronStorageIndex],inputStorageIndex);

    // Check if all inputs required by that specific neuron have been received
    if(allBits(receivedInputs[neuronStorageIndex], inputSize)){
        // If all inputs required by the neuron have been received, proceed with output computation
        neuronOutput = computeNeuronOutput(neuronId);
        outputValues[neuronStorageIndex] = neuronOutput;

        //LOG(APP,DEBUG,"Output inside function:%f\n",neuronOutput);
        //reset the bit field for the next NN run
        resetAll(receivedInputs[neuronStorageIndex]);

        //TODO Send the output for the nodes that need him
    }

}

void encodeNeuronOutputMessage(char* messageBuffer,size_t bufferSize,int outputNeuronId, float neuronOutput,int* inputNeuronsIds,int nNeurons){
    int offset = 0;
    //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron ID] [Input Neuron Number 1] [Input Neuron Number 2] ... [Input Neuron Number N] [Output Value]
    //Encode the neuron id that generated this output
    offset = snprintf(messageBuffer,bufferSize,"%d %d ",NN_NEURON_OUTPUT,outputNeuronId);

    // Encode the IDs of the input neurons that require this output
    for (int i = 0; i < nNeurons; i++) {
        offset += snprintf(messageBuffer+offset,bufferSize-offset,"%d ",inputNeuronsIds[i]);
    }

    // Encode the computed output value
    offset += snprintf(messageBuffer+offset,bufferSize-offset,"%g",neuronOutput);

}

void encodeNACKMessage(char* messageBuffer, size_t bufferSize,int* missingNeuronInputs, int missingNeuronCount){
    int offset = 0;
    //DATA_MESSAGE NN_NACK [Neuron ID with Missing Output] [Missing Output ID 1] [Missing Output ID 2] ...

    offset = snprintf(messageBuffer,bufferSize,"%d ",NN_NACK);

    // Encode the IDs of neurons whose required inputs are missing
    for (int i = 0; i < missingNeuronCount; i++) {
        offset += snprintf(messageBuffer+offset,bufferSize-offset,"%d ",missingNeuronInputs[i]);
    }
}

void encodeACKMessage(char* messageBuffer, size_t bufferSize,int* neuronAckList, int ackNeuronCount){
    int offset = 0;
    //NN_ACK [Acknowledged Neuron ID 1] [Acknowledged Neuron ID 2]...

    offset = snprintf(messageBuffer,bufferSize,"%d ",NN_ACK);

    // Encode the IDs of neurons whose required inputs are missing
    for (int i = 0; i < ackNeuronCount; i++) {
        offset += snprintf(messageBuffer+offset,bufferSize-offset,"%d ",neuronAckList[i]);
    }

}