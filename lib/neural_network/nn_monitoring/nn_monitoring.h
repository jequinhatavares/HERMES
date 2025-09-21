#ifndef NN_MONITORING_H
#define NN_MONITORING_H

#include "app_globals.h"


enum NNMonitoringMessageType{
    NEURAL_NETWORK_SETUP_TIME,          //0
    NEURAL_NETWORK_INFERENCE_RESULTS,   //1
};


void reportSetupTime(unsigned long nnSetUpTime, uint8_t missingACKs);
void reportInferenceResults(int inferenceId, unsigned long nnInferenceTime, int nackCount,float* outputValues, int nOutputs);

#endif //NN_MONITORING_H
