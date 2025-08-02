#include "neural_network_dispatcher.h"


#ifdef ESP8266
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

#ifdef ESP32
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

#ifdef raspberrypi_3b
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

// For unit test on native platform
#ifdef NATIVE
DeviceType deviceType = DeviceType::DEVICE_ESP8266;
#endif

char appPayload[200];
char appBuffer[250];

class Network network;

void onNetworkJoin(uint8_t *parentIP){
    uint8_t myIP[4];
    network.getNodeIP(myIP);
    // Send a message to the root node notifying it that this node is a potential neural network worker,
    // including the device class (e.g., ESP8266, NodeMCU, or Raspberry Pi).
    encodeWorkerRegistration(appPayload, sizeof(appPayload),myIP,deviceType);
    network.sendMessageToRoot(appBuffer, sizeof(appBuffer),appPayload);
}


void handleNeuralNetworkMessage(uint8_t* originatorIP,uint8_t* destinationIP,char* messageBuffer){
    NeuralNetworkMessageType type;

    sscanf(messageBuffer, "%d",&type);
    switch (type) {
        case NN_WORKER_REGISTRATION:
            if(network.iamRoot) handleACKMessage(messageBuffer);
            else LOG(APP,ERROR,"This node received a message meant for the coordinator (root) node. Message: %s", messageBuffer);
            break;
        case NN_ASSIGN_COMPUTATION:
            handleNeuronMessage(messageBuffer);
            break;
        case NN_ASSIGN_OUTPUT_TARGETS:
            handleNeuronMessage(messageBuffer);
            break;
        case NN_NEURON_OUTPUT:
            handleNeuronMessage(messageBuffer);
            break;
        case NN_FORWARD:
            handleNeuronMessage(messageBuffer);
            break;
        case NN_NACK:
            handleNeuronMessage(messageBuffer);
            break;
        case NN_ACK:
            if(network.iamRoot) handleACKMessage(messageBuffer);
            else LOG(APP,ERROR,"This node received a message meant for the coordinator (root) node. Message: %s", messageBuffer);
            break;
        default:
            break;
    }

}

void neuralNetworkOnTimer(){
    /*** Check if all expected inputs for each neuron have been received.
     * If any inputs are missing and the timeout has elapsed, trigger the NACK mechanism.
     * Also check if previously triggered NACKs have expired, allowing the neuron to compute its output even with missing inputs.***/
    manageNeuron();

    /*** Check if the network has the required number of nodes to start neural network computation assignments.
     * Once all neurons have been assigned to physical nodes, wait for acknowledgments from those devices.
     * If the timeout expires and some neurons remain unacknowledged, resend the assignments for those neurons.***/
    if(network.iamRoot) manageNeuralNetwork();

}
