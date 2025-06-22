#include "neuron_manager.h"

BitField receivedInputs[MAX_NEURONS];

void handleNeuralNetworkMessage(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    int neuronId,inputSize,*inputIndexMap,outputNeuron;
    float bias, *weightValues,inputValue;
    char* token, *spaceToken,*neuronEntry;
    char *saveptr1, *saveptr2;

    switch (type) {
        case NN_ASSIGN_COMPUTATION:
            //DATA_MESSAGE NN_ASSIGN_COMPUTATION |[Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
            neuronEntry = strtok_r(messageBuffer, "|",&saveptr1);
            //Discard the message types
            neuronEntry = strtok_r(NULL, "|",&saveptr1);

            while (neuronEntry != nullptr){
                LOG(NETWORK,DEBUG," neuron entry token:%s\n",neuronEntry);

                // Position the spaceToken pointer at the beginning of the current neuron's data segment
                spaceToken = strtok_r(neuronEntry, " ",&saveptr2);

                //spaceToken now pointing to the Neuron number
                neuronId = atoi(spaceToken);
                LOG(NETWORK,DEBUG," neuronId token:%s\n",spaceToken);

                //spaceToken now pointing to the input size
                spaceToken = strtok_r(NULL, " ", &saveptr2);
                inputSize = atoi(spaceToken);
                LOG(NETWORK,DEBUG," inputSize token:%s\n",spaceToken);

                inputIndexMap = new int[inputSize];
                weightValues = new float[inputSize];

                //spaceToken now pointing to the Input Save Order Vector
                spaceToken = strtok_r(NULL, " ", &saveptr2);
                for (int i = 0; i < inputSize; ++i) {
                    inputIndexMap[i]= atoi(spaceToken);
                    LOG(NETWORK,DEBUG," inputIndexMap token:%s\n",spaceToken);
                    //Move on the next input to index map
                    spaceToken = strtok_r(NULL, " ", &saveptr2);
                }

                //spaceToken now pointing to the weights Vector
                for (int i = 0; i < inputSize; ++i) {
                    weightValues[i]=atof(spaceToken);
                    LOG(NETWORK,DEBUG," weightValues token:%s\n",spaceToken);
                    //Move on the next input to index map
                    spaceToken = strtok_r(NULL, " ", &saveptr2);
                }

                //spaceToken now pointing to the bias value
                bias=atof(spaceToken);
                LOG(NETWORK,DEBUG," bias token:%s\n",spaceToken);

                //Save the parsed neuron parameters
                configureNeuron(neuronId,inputSize,weightValues,bias, inputIndexMap);

                delete[] inputIndexMap;
                delete[] weightValues;

                //Move on to the next neuron
                neuronEntry = strtok_r(NULL, "|",&saveptr1);
            }
            break;

        case NN_NEURON_OUTPUT:
            //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron Number] [Input Neuron Number] [Output Value]
            sscanf(messageBuffer, "%*d %*d %d %d %f",&outputNeuron,&neuronId,&inputValue);
            setInput(neuronId,inputValue,outputNeuron);
            break;
        default:
            break;
    }
}

void handleNeuronInput(int neuronId,int outputNeuronId){
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

    // Set the bit corresponding to the received input to 1
    setBit(receivedInputs[neuronStorageIndex],inputStorageIndex);

    if(allBits(receivedInputs[neuronStorageIndex], inputSize)){
        neuronOutput = computeNeuronOutput(neuronId);

        //reset the bit field for the next NN run
        resetAll(receivedInputs[neuronStorageIndex]);

        //TODO Send the output for the nodes that need him
    }

}

/**
 * setBit
 * Sets the i-th bit (0-indexed from LSB) in the bit field
 *
 * @param bits - Reference to bit field to modify
 * @param i    - Bit position to set (0-indexed)
 **/
inline void setBit(BitField& bits, uint8_t i) {
    bits |= (1U << i); //1U is the unsigned integer value 1-> 0b00000001
}

/**
 * countBits
 * Counts the number of set bits in the bit field
 *
 * @param bits - Bit field to examine
 * @return Number of bits set to 1
 **/
inline uint8_t countBits(BitField bits) {
    return __builtin_popcount(bits); // GCC/Clang intrinsic, fast!
}

/**
 * allBits
 * Checks if the lowest n bits are all set in the bit field
 *
 * @param bits - Bit field to check
 * @param n    - Number of lowest bits to verify
 * @return True if all n bits are set, false otherwise
 **/
inline bool allBits(BitField bits, uint8_t n) {
    return bits == ((1U << n) - 1);// Check if the lowest 'n' bits are all set to 1 (i.e., bits == 2^n - 1)
}

/**
 * resetAll
 * Clears all bits in the bit field (sets to 0)
 *
 * @param bits - Reference to bit field to reset
 **/
inline void resetAll(BitField& bits){
    bits = 0;
}