#include "neuron_manager.h"


void handleNeuralNetworkMessage(char* messageBuffer){
    NeuralNetworkMessageType type;
    sscanf(messageBuffer, "%*d %d",&type);
    int neuronNumber,inputSize,*inputIndexMap,outputNeuron;
    float bias, *weightValues,inputValue;
    char* token, *spaceToken,*entry;
    char *saveptr1, *saveptr2;

    switch (type) {
        case NN_ASSIGN_COMPUTATION:
            //DATA_MESSAGE NN_ASSIGN_COMPUTATION [Neuron Number] [Input Size] [Input Save Order] [weights values] [bias]
            sscanf(messageBuffer, "%*d %*d %d %d",&neuronNumber,&inputSize);
            inputIndexMap = new int[inputSize];
            weightValues = new float[inputSize];

            token = strtok_r(messageBuffer, " ",&saveptr1);
            //LOG(NETWORK,DEBUG,"token:%s\n",token);

            //Discard space between message types
            token = strtok_r(NULL, " ", &saveptr1);
            //LOG(NETWORK,DEBUG,"token:%s\n",token);
            //Discard the neuron number
            token = strtok_r(NULL, " ", &saveptr1);
            //LOG(NETWORK,DEBUG,"token:%s\n",token);
            //Discard the input size
            token = strtok_r(NULL, " ", &saveptr1);

            //Discard the input size
            token = strtok_r(NULL, " ", &saveptr1);

            for (int i = 0; i < inputSize; ++i) {
                inputIndexMap[i]= atoi(token);
                //LOG(NETWORK,DEBUG,"token in inputIndexMap:%s\n",token);
                //Move on the next input to index map
                token = strtok_r(NULL, " ", &saveptr1);
            }

            for (int i = 0; i < inputSize; ++i) {
                weightValues[i]=atof(token);
                //LOG(NETWORK,DEBUG,"token in weightValues:%s\n",token);
                //Move on the next input to index map
                token = strtok_r(NULL, " ", &saveptr1);
            }

            //LOG(NETWORK,DEBUG,"token in bias:%s\n",token);
            bias=atof(token);

            configureNeuron(inputSize,weightValues,bias, inputIndexMap);

            delete[] inputIndexMap;
            delete[] weightValues;

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