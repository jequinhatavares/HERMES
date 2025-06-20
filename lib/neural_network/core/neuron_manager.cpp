#include "neuron_manager.h"


void handleNeuralNetworkMessage(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    int neuronNumber,inputSize,*inputIndexMap,outputNeuron;
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
                neuronNumber = atoi(spaceToken);
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

                //Save the parsed neuron values
                configureNeuron(inputSize,weightValues,bias, inputIndexMap);

                delete[] inputIndexMap;
                delete[] weightValues;

                //Move on to the next neuron
                neuronEntry = strtok_r(NULL, "|",&saveptr1);
            }
            break;

        case NN_NEURON_OUTPUT:
            //DATA_MESSAGE NN_NEURON_OUTPUT [Output Neuron Number] [Input Neuron Number] [Output Value]
            sscanf(messageBuffer, "%*d %*d %d %d %f",&outputNeuron,&neuronNumber,&inputValue);
            setInput(inputValue,outputNeuron);
            break;
        default:
            break;
    }
}

void handleNeuronInput(){

}