#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <cstring>
#include <logger.h>

extern int* saveOrder;
extern float* weights;
extern float* inputs;

void configureNeuron(int receivedInputSize, float* receivedWeights, float receivedBias, int* receivedOrder);
float computeNeuronOutput();
void setInput(float inputValue, int inputID);

#endif //NEURAL_NETWORK_H
