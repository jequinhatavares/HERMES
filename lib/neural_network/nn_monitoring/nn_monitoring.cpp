#include "nn_monitoring.h"

char appTmpMonitoringBuffer[50];

void reportSetupTime(unsigned long nnSetUpTime){
    //NEURAL_NETWORK_SETUP_TIME [NN Setup Time]
    snprintf(appTmpMonitoringBuffer, sizeof(appTmpMonitoringBuffer),"%d %lu",NEURAL_NETWORK_SETUP_TIME,nnSetUpTime);
    monitoring.encodeAppLevelMessage(appTmpMonitoringBuffer,monitoring.monitoringBuffer, sizeof(monitoring.monitoringBuffer));
    monitoring.reportAppLevelMonitoringMessage(monitoring.monitoringBuffer);
}

void reportInferenceResults(int inferenceId, unsigned long nnInferenceTime, int nackCount,float* outputValues, int nOutputs){
    int nChars=0;
    int strategyType = network.getActiveMiddlewareStrategy();

    //NEURAL_NETWORK_INFERENCE_TIME [Strategy Type] [Inference Id] [NN Inference Time] [NACK Count] [N outputs] [Output Value 1] ... [Output Value N]
    nChars = snprintf(appTmpMonitoringBuffer, sizeof(appTmpMonitoringBuffer),"%d %d %d %lu %d %d",NEURAL_NETWORK_INFERENCE_RESULTS,strategyType,inferenceId,nnInferenceTime,nackCount,nOutputs);

    for (int i = 0; i < nOutputs; i++) {
        nChars += snprintf(appTmpMonitoringBuffer+nChars, sizeof(appTmpMonitoringBuffer)-nChars," %f",outputValues[i]);
    }

    monitoring.encodeAppLevelMessage(appTmpMonitoringBuffer,monitoring.monitoringBuffer, sizeof(monitoring.monitoringBuffer));
    monitoring.reportAppLevelMonitoringMessage(monitoring.monitoringBuffer);
}
