#include "neuron_core.h"

int* saveOrder = nullptr;
float* weights = nullptr;
float* inputs = nullptr;
int inputSize = 0;
float bias = 0.0f;

void configureNeuron(int receivedInputSize, float* receivedWeights, float receivedBias, int* receivedOrder) {
    // Clean up previous allocations
    // if (weights) delete[] weights;
    // Initialize the input size (equal to the number of neurons in the previous layer)    inputSize = receivedInputSize;
    inputSize = receivedInputSize;
    weights = new float[inputSize];
    memcpy(weights, receivedWeights, sizeof(float) * inputSize);

    //Initialize the node bias value
    bias = receivedBias;

    //if (inputs) delete[] inputs;
    // Allocate space to store inputs (that correspond to the outputs of the previous layers)
    inputs = new float[inputSize];

    //if (saveOrder) delete[] saveOrder;
    // Allocate space to store the save order
    saveOrder = new int[inputSize];
    memcpy(saveOrder, receivedOrder, sizeof(int) * inputSize);/******/

}

void setInput(float inputValue, int inputID){
    int saveIndex = -1;
    for (int i = 0; i < inputSize; ++i) {
        if(saveOrder[i]==inputID){
           saveIndex = i;
        }
    }
    LOG(NETWORK,DEBUG,"save index:%i\n",saveIndex);
    if(saveIndex != -1){
        inputs[saveIndex] = inputValue;
    }
}

float ReLu(float x){
    return x > 0 ? x : 0;
}


float computeNeuronOutput(){
    float sum = bias;
    for (int i = 0; i < inputSize; i++) {
        sum += inputs[i] * weights[i];
    }
    return ReLu(sum);  // Activation function
}


