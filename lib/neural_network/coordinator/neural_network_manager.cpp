#include "neural_network_manager.h"


void encodeAssignComputationMessage(char* messageBuffer, size_t bufferSize, int neuronId, int inputSize, int* inputSaveOrder,float*weightsValues, float bias){
    //NN_ASSIGN_COMPUTATION [Neuron Number 1] [Input Size] [Input Save Order] [weights values] [bias]
    int offset = 0;

    //Encode: NN_ASSIGN_COMPUTATION [Neuron Number 1] [Input Size]
    offset = snprintf(messageBuffer,bufferSize,"%i %i %i ",NN_ASSIGN_COMPUTATION,neuronId,inputSize);

    //Encode the [Input Save Order] vector
    for (int i = 0; i < inputSize; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%i ",inputSaveOrder[i]);
    }

    //Encode the [weights values] vector
    for (int i = 0; i < inputSize; i++) {
        //%g format specifier automatically chooses the most compact representation, removing unnecessary trailing zeros
        //2.0 -> 2 , 2.1->2.1
        offset += snprintf(messageBuffer + offset, bufferSize - offset,"%g ",weightsValues[i]);
    }

    offset += snprintf(messageBuffer + offset, bufferSize - offset,"%g",bias);

}